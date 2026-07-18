/**
  @file SrRenderer.cpp
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#include "StdAfx.h"

#include <d3d9.h>

#include "SrSoftRenderer.h"
#include "srBitmap.h"
#include "SrRasterizer.h"
#include "SrProfiler.h"
#include "SrSwShader.h"
#include "SrFragmentBuffer.h"
#include <gl/GL.h>
#include <fstream>

#include "mmgr/mmgr.h"

#define SR_NORMALIZE_VB_MAX_SIZE 1024 * 1024 * 10

namespace
{
	struct SrCachedGlyph
	{
		SrCachedGlyph() : width(0), height(0), originX(0), originY(0), advance(0), stride(0) {}
		uint32 width, height;
		int originX, originY, advance;
		uint32 stride;
		std::vector<uint8> bits;
	};

	struct SrCachedFont
	{
		SrCachedFont() : ready(false), ascent(0) {}
		bool ready;
		int ascent;
		SrCachedGlyph glyphs[128];
	};

	SrCachedFont g_bigFontCache;
	SrCachedFont g_smallFontCache;

	static bool BuildFontCache(HFONT font, SrCachedFont& cache)
	{
		HDC dc = CreateCompatibleDC(NULL);
		if (!dc)
			return false;
		HGDIOBJ oldFont = SelectObject(dc, font);
		TEXTMETRIC metrics;
		memset(&metrics, 0, sizeof(metrics));
		GetTextMetrics(dc, &metrics);
		cache.ascent = metrics.tmAscent;
		MAT2 transform;
		memset(&transform, 0, sizeof(transform));
		transform.eM11.value = 1;
		transform.eM22.value = 1;
		for (uint32 character = 32; character < 128; ++character)
		{
			GLYPHMETRICS glyphMetrics;
			memset(&glyphMetrics, 0, sizeof(glyphMetrics));
			const DWORD size = GetGlyphOutlineA(dc, character, GGO_BITMAP, &glyphMetrics, 0, NULL, &transform);
			if (size == GDI_ERROR)
				continue;
			SrCachedGlyph& glyph = cache.glyphs[character];
			glyph.width = glyphMetrics.gmBlackBoxX;
			glyph.height = glyphMetrics.gmBlackBoxY;
			glyph.originX = glyphMetrics.gmptGlyphOrigin.x;
			glyph.originY = glyphMetrics.gmptGlyphOrigin.y;
			glyph.advance = glyphMetrics.gmCellIncX;
			glyph.stride = ((glyph.width + 31) / 32) * 4;
			glyph.bits.resize(size);
			if (size)
				GetGlyphOutlineA(dc, character, GGO_BITMAP, &glyphMetrics, size, &glyph.bits[0], &transform);
		}
		SelectObject(dc, oldFont);
		DeleteDC(dc);
		cache.ready = true;
		return true;
	}

	static uint64_t HashBytes(uint64_t hash, const void* data, size_t size)
	{
		const uint8* bytes = static_cast<const uint8*>(data);
		for (size_t i = 0; i < size; ++i)
		{
			hash ^= bytes[i];
			hash *= 1099511628211ULL;
		}
		return hash;
	}
}

SrSoftRenderer::SrSoftRenderer(void):IRenderer(eRt_Software),
	m_d3d9(NULL),
	m_drawSurface(NULL),
	m_hwDevice(NULL),
	m_cachedBuffer(NULL),
	m_bufferPitch(0),
	m_renderState(0),
	m_normalizeVertexBuffer(NULL),
	m_currShader(NULL),
	m_shaderConstants(NULL),
	m_rasterizer(NULL)
{
	m_textureStages.assign( SR_MAX_TEXTURE_STAGE_NUM , NULL );
	m_normalizeVBAllocSize = 0;

	// create HFONT
	LOGFONT lfont;
	memset   (&lfont,   0,   sizeof   (LOGFONT));   
	lfont.lfHeight=14;
	lfont.lfWeight=800;   
	lfont.lfClipPrecision=CLIP_LH_ANGLES; 
	lfont.lfQuality = NONANTIALIASED_QUALITY; // THIS COULD BOOST
	strcpy_s( lfont.lfFaceName, "verdana" );
	m_bigFont = CreateFontIndirect( &lfont );

	lfont.lfHeight=12;
	lfont.lfWeight=0;  
	m_smallFont = CreateFontIndirect( &lfont );

}

SrSoftRenderer::~SrSoftRenderer(void)
{
	DeleteObject(m_smallFont);
	DeleteObject(m_bigFont);
}

bool SrSoftRenderer::InitRenderer( HWND hWnd, int width, int height, int bpp )
{
	//////////////////////////////////////////////////////////////////////////
	// create d3d device for Show Soft Buffer
	if( NULL == ( m_d3d9 = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		return false;

	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	// bpp 32 XRGB
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	// Create the D3DDevice
	if( FAILED( m_d3d9->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &m_hwDevice ) ) )
	{
		MessageBoxA(hWnd, "Unable to create D3D device", "", MB_OK);
		return false;
	}

	// Get backBuffrt
	m_hwDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_drawSurface);
	BuildFontCache(m_bigFont, g_bigFontCache);
	BuildFontCache(m_smallFont, g_smallFontCache);

	// ������դ��������
	m_rasterizer = new SrRasterizer;
	m_rasterizer->Init(this);

	m_normalizeVertexBuffer = (SrRendVertex*)_mm_malloc_custom( SR_NORMALIZE_VB_MAX_SIZE * sizeof(SrRendVertex), 16 );

	m_shaderConstants = (float4*)_mm_malloc_custom( eSC_ShaderConstantCount * sizeof(float4), 16 );
	memset(m_shaderConstants, 0, eSC_ShaderConstantCount * sizeof(float4));

	InnerInitShaders();

	return true;
}

bool SrSoftRenderer::ShutdownRenderer()
{
	for (uint32 i=0; i < m_swHandles.size(); ++i)
	{
		FreeLibrary( m_swHandles[i] );
	}
	m_swHandles.clear();

	_mm_free_custom(m_normalizeVertexBuffer);
	m_normalizeVertexBuffer = 0;
	_mm_free_custom(m_shaderConstants);
	m_shaderConstants = 0;

	SAFE_DELETE( m_rasterizer );

	SAFE_RELEASE( m_drawSurface );
	SAFE_RELEASE( m_hwDevice );
	SAFE_RELEASE( m_d3d9 );

	return true;
}

bool SrSoftRenderer::HwClear()
{
// 	if (!m_drawSurface)
// 		return false;
// 
// 	HRESULT res = m_hwDevice->ColorFill(m_drawSurface,0,D3DCOLOR_ARGB(255,50,50,50));
// 	if (res!=S_OK)
// 	{
// 		return false;
// 	}

	return true;
}

bool SrSoftRenderer::Swap()
{
	m_renderState |= eRS_Swaping;

	m_hwDevice->Present(NULL, NULL, NULL, NULL);

	m_renderState &= ~eRS_Swaping;
	return true;
}

void SrSoftRenderer::BeginFrame()
{
	m_renderState |= eRs_Rendering;
	m_frameCount++;

	ClearTextureStage();
}

void SrSoftRenderer::EndFrame()
{
		D3DLOCKED_RECT lockinfo;
		memset(&lockinfo,0,sizeof(lockinfo));

		HRESULT res = m_drawSurface->LockRect(&lockinfo,NULL,D3DLOCK_DISCARD);
		if (res!=S_OK)
		{
			// FATAL ERROR
			return;			
		}

		m_cachedBuffer = lockinfo.pBits;
		m_bufferPitch = lockinfo.Pitch;
		m_rasterizer->Flush();

		const char* hashOutput = getenv("SR_HASH_OUTPUT");
		if (hashOutput && hashOutput[0])
		{
			uint64_t colorHash = 1469598103934665603ULL;
			uint64_t depthHash = 1469598103934665603ULL;
			uint64_t gbufferHash = 1469598103934665603ULL;
			uint64_t winnerHash = 1469598103934665603ULL;
			uint32 activePixels = 0;
			for (int y = 0; y < g_context->height; ++y)
				colorHash = HashBytes(colorHash, static_cast<const uint8*>(m_cachedBuffer) + y * m_bufferPitch,
					g_context->width * sizeof(uint32));
			const uint32 pixelCount = g_context->width * g_context->height;
			depthHash = HashBytes(depthHash, fBuffer->zBuffer, pixelCount * sizeof(float));
			for (uint32 i = 0; i < pixelCount; ++i)
			{
				if (fBuffer->zBuffer[i] < 0.f) ++activePixels;
				gbufferHash = HashBytes(gbufferHash, fBuffer->fBuffer[i].data, sizeof(fBuffer->fBuffer[i].data));
				const uint32 winner = fBuffer->fBuffer[i].primitive ? fBuffer->fBuffer[i].drawSequence : 0xffffffffu;
				winnerHash = HashBytes(winnerHash, &winner, sizeof(winner));
			}
			std::ifstream existing(hashOutput, std::ios::binary | std::ios::ate);
			const bool emptyOutput = !existing || existing.tellg() == 0;
			existing.close();
			const std::vector<SrWorkerFrameStats>& workerStats = gEnv->profiler->getWorkerStats();
			uint32 workerJobs = 0;
			uint32 activeWorkers = 0;
			float workerBusySum = 0.f;
			float workerBusyMax = 0.f;
			for (uint32 i = 0; i < workerStats.size(); ++i)
			{
				workerJobs += workerStats[i].jobs;
				workerBusySum += workerStats[i].busyMilliseconds;
				workerBusyMax = (std::max)(workerBusyMax, workerStats[i].busyMilliseconds);
				if (workerStats[i].jobs > 0) ++activeWorkers;
			}
			const float workerBusyMean = activeWorkers ? workerBusySum / activeWorkers : 0.f;
			const float flushMilliseconds = gEnv->profiler->getTime(ePe_FlushTime);
			const float workerBusyRatio = flushMilliseconds > 0.f && !workerStats.empty()
				? workerBusySum / (flushMilliseconds * workerStats.size()) : 0.f;
			std::ofstream stream(hashOutput, std::ios::app);
			if (emptyOutput)
				stream << "frame,color_hash,depth_hash,gbuffer_hash,winner_hash,active_pixels,flush_ms,vertex_ms,raster_ms,pixel_ms,bin_ms,bin_bounds_ms,bin_count_ms,bin_prefix_ms,bin_scatter_ms,tile_size,tile_count,tile_jobs,relaxed_ssao,active_tiles,empty_tiles,bin_refs,batches,worker_jobs,active_workers,worker_busy_ms,worker_busy_max_ms,worker_busy_ratio,worker_balance,tile_busy_ratio,tile_worker_balance\n";
			stream << m_frameCount << ',' << colorHash << ',' << depthHash << ',' << gbufferHash << ',' << winnerHash << ',' << activePixels << ','
				<< gEnv->profiler->getTime(ePe_FlushTime) << ','
				<< gEnv->profiler->getTime(ePe_VertexShaderTime) << ','
				<< gEnv->profiler->getTime(ePe_RasterizeShaderTime) << ','
				<< gEnv->profiler->getTime(ePe_PixelShaderTime) << ','
				<< gEnv->profiler->getTime(ePe_BinningTime) << ','
				<< gEnv->profiler->getTime(ePe_BinBoundsTime) << ','
				<< gEnv->profiler->getTime(ePe_BinCountTime) << ','
				<< gEnv->profiler->getTime(ePe_BinPrefixTime) << ','
				<< gEnv->profiler->getTime(ePe_BinScatterTime) << ','
				<< gEnv->profiler->getCount(ePe_TileSize) << ','
				<< gEnv->profiler->getCount(ePe_TileCount) << ','
				<< gEnv->profiler->getCount(ePe_TileJobCount) << ','
				<< gEnv->profiler->getCount(ePe_RelaxedSsao) << ','
				<< gEnv->profiler->getCount(ePe_ActiveTileCount) << ','
				<< gEnv->profiler->getCount(ePe_EmptyTileCount) << ','
				<< gEnv->profiler->getCount(ePe_BinReferenceCount) << ','
				<< gEnv->profiler->getCount(ePe_BatchCountTile) << ','
				<< workerJobs << ',' << activeWorkers << ',' << workerBusySum << ',' << workerBusyMax << ','
				<< workerBusyRatio << ',' << (workerBusyMean > 0.f ? workerBusyMax / workerBusyMean : 0.f) << ','
				<< gEnv->profiler->getTileBusyRatio() << ',' << gEnv->profiler->getTileWorkerBalance() << '\n';
		}

		FlushText();
		m_drawSurface->UnlockRect();
 
		m_textLines.clear();

	m_renderState &= ~eRs_Rendering;

	Swap();
	for (uint32 i = 0; i < m_normlizedVBs.size(); ++i)
		delete m_normlizedVBs[i];
	m_normlizedVBs.clear();

	m_normalizeVBAllocSize = 0;
}

void* SrSoftRenderer::getBuffer()
{
	return m_cachedBuffer;
}

bool SrSoftRenderer::SetTextureStage( const SrTexture* texture, int stage )
{
	if( texture && stage < SR_MAX_TEXTURE_STAGE_NUM )
	{
		m_textureStages[stage] = texture;
		return true;
	}
	
	return false;
}


void SrSoftRenderer::ClearTextureStage()
{
	for (uint32 i=0; i < m_textureStages.size(); ++i)
	{
		m_textureStages[i] = NULL;
	}
}

bool SrSoftRenderer::DrawPrimitive( SrPrimitve* primitive )
{
	return m_rasterizer->DrawPrimitive(primitive);
}

void SrSoftRenderer::FlushText()
{
	if (std::getenv("SR_HEADLESS") || std::getenv("SR_DISABLE_UI"))
		return;
	if (m_textLines.empty() || !m_cachedBuffer)
		return;

	gEnv->profiler->setBegin(ePe_FlushTextTime);
	for (SrTextLines::const_iterator it = m_textLines.begin(); it != m_textLines.end(); ++it)
	{
		const SrCachedFont& font = it->size == 0 ? g_bigFontCache : g_smallFontCache;
		if (!font.ready)
			continue;
		const uint32 color = (GetRValue(it->color) << 16) | (GetGValue(it->color) << 8) | GetBValue(it->color);
		int penX = it->pos.x;
		const int baseline = it->pos.y + font.ascent;
		for (size_t characterIndex = 0; characterIndex < it->text.size(); ++characterIndex)
		{
			uint32 character = static_cast<uint8>(it->text[characterIndex]);
			if (character < 32 || character >= 128)
				character = '?';
			const SrCachedGlyph& glyph = font.glyphs[character];
			const int glyphX = penX + glyph.originX;
			const int glyphY = baseline - glyph.originY;
			if (!glyph.stride || glyph.bits.size() < static_cast<size_t>(glyph.stride) * glyph.height)
			{
				penX += glyph.advance;
				continue;
			}
			for (uint32 row = 0; row < glyph.height; ++row)
			{
				const int y = glyphY + static_cast<int>(row);
				if (y < 0 || y >= g_context->height)
					continue;
				uint32* destination = reinterpret_cast<uint32*>(static_cast<uint8*>(m_cachedBuffer) + y * m_bufferPitch);
				const uint8* source = &glyph.bits[row * glyph.stride];
				for (uint32 column = 0; column < glyph.width; ++column)
				{
					const int x = glyphX + static_cast<int>(column);
					if (x >= 0 && x < g_context->width && (source[column >> 3] & (0x80 >> (column & 7))))
						destination[x] = color;
				}
			}
			penX += glyph.advance;
		}
	}
	gEnv->profiler->setEnd(ePe_FlushTextTime);
}



bool SrSoftRenderer::Resize( uint32 width, uint32 height )
{
	return true;
}

uint32 SrSoftRenderer::getWidth()
{
	return g_context->width;
}

uint32 SrSoftRenderer::getHeight()
{
	return g_context->height;
}

bool SrSoftRenderer::DrawLine( const float3& from, const float3& to )
{
	m_rasterizer->DrawLine(from, to);
	return true;
}

SrVertexBuffer* SrSoftRenderer::AllocateNormalizedVertexBuffer( uint32 count, bool fastmode )
{
	if (fastmode)
	{
		SrVertexBuffer* ret = new SrVertexBuffer;
		ret->data = (uint8*)(m_normalizeVertexBuffer + m_normalizeVBAllocSize);
		ret->elementCount = count;
		ret->elementSize = sizeof(SrRendVertex);
		m_normlizedVBs.push_back(ret);

		m_normalizeVBAllocSize += count;

		return ret;
	}
	else
	{
		return gEnv->resourceMgr->AllocateVertexBuffer( sizeof(SrRendVertex), count);
	}	
}

bool SrSoftRenderer::SetShader( const SrShader* shader )
{
	if (shader)
	{
		for ( uint32 i=0; i < m_swShaders.size(); ++i )
		{
			if (m_swShaders[i] && m_swShaders[i]->m_bindShader == shader)
			{
				m_currShader = m_swShaders[i];
				return true;
			}
		}
	}
	
	return true;
}

bool SrSoftRenderer::SetShaderConstant( uint32 slot, const float* constantStart, uint32 vec4Count )
{
	memcpy( &(m_shaderConstants[slot]), constantStart, vec4Count * sizeof(float4) );
	return true;
}

uint32 SrSoftRenderer::Tex2D( float2& texcoord, const SrTexture* texture  ) const
{
	uint32 ret = 0x00000000;
	if (texture)
	{
		if (g_context->IsFeatureEnable(eRFeature_LinearFiltering))
		{
			ret = texture->Get( texcoord, eSF_Linear );
		}
		else
		{
			ret = texture->Get( texcoord, eSF_Nearest );
		}
	}	

	return ret;
}

typedef void (*fnModuleInit)(GlobalEnvironment* pgEnv);
typedef SrSwShader* (*fnLoadShader)(const char* shaderName);

bool SrSoftRenderer::InnerInitShaders()
{
	m_swShaders.clear();

	std::string dir = "\\shader\\";
	std::string path = "\\shader\\*.swsl";
	getMediaPath(dir);
	getMediaPath(path);
	 
	WIN32_FIND_DATAA fd;
	HANDLE hff = FindFirstFileA(path.c_str(), &fd);
	BOOL bIsFind = TRUE;
	 
	while(hff && bIsFind)
	{
	 	if(fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
	 	{
	 		// do not get into
	 	}
	 	else
	 	{
	 		std::string fullpath = dir + fd.cFileName;
	 
	 		// load dll shaders
	 		HMODULE hDllHandle = 0;
	 		hDllHandle = LoadLibraryA( fullpath.c_str() );
	 		if (hDllHandle)
	 		{
				fnModuleInit fnCount = (fnModuleInit)(GetProcAddress( hDllHandle, "ModuleInit" ));
	 			fnLoadShader fnLoad = (fnLoadShader)(GetProcAddress( hDllHandle, "LoadShader" ));
	 
				fnCount(gEnv);

				// add other shaders
				SrResourceManager* resMng = (SrResourceManager*)gEnv->resourceMgr;
				SrResourceLibrary::iterator it = resMng->m_shaderLibrary.begin();
				for (; it != resMng->m_shaderLibrary.end(); ++it)
				{
					SrSwShader* shader = fnLoad(it->second->getName());
					if (shader)
					{
						shader->m_bindShader = reinterpret_cast<SrShader*>(it->second);
						m_swShaders.push_back(shader);
					}
					
				}

 	 			m_swHandles.push_back(hDllHandle);
	 		}		
	 	}
	 	bIsFind = FindNextFileA(hff, &fd);
	}


	return true;
}

bool SrSoftRenderer::DrawScreenText( const char* str, int x,int y, uint32 size, DWORD color /*= SR_UICOLOR_HIGHLIGHT*/ )
{
	SrTextLine line;
	line.text = std::string(str);
	line.pos = int2(x,y);
	line.size = size;
	line.color = color;

	m_textLines.push_back( line );

	return true;
}

const char* SrSoftRenderer::getName()
{
	return "SwCPU";
}
