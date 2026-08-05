/**
  @file SrRenderer.cpp
  
  @author yikaiming

  锟斤拷锟斤拷锟斤拷志 history
  ver:1.0
   
 */

#include "stdafx.h"

#include <filesystem>

#include "SrSoftRenderer.h"
#include "SrBitmap.h"
#include "SrRasterizer.h"
#include "SrProfiler.h"
#include "SrSwShader.h"
#include "SrFragmentBuffer.h"
#include "SrPerformance.h"
#include <fstream>

#include "mmgr/mmgr.h"

#define SR_NORMALIZE_VB_MAX_SIZE 1024 * 1024 * 10

namespace
{
	SrVertexBufferArray g_inFlightNormalizedVBs;
	uint32 g_inFlightFrameNumber = 0;

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

	static void WriteFrameHash(uint32 frameNumber, const void* colorBuffer, int colorPitch)
	{
		const char* hashOutput = getenv("SR_HASH_OUTPUT");
		if (!hashOutput || !hashOutput[0])
			return;
		uint64_t colorHash = 1469598103934665603ULL;
		uint64_t depthHash = 1469598103934665603ULL;
		uint64_t gbufferHash = 1469598103934665603ULL;
		uint64_t winnerHash = 1469598103934665603ULL;
		uint32 activePixels = 0;
		for (int y = 0; y < g_context->height; ++y)
			colorHash = HashBytes(colorHash, static_cast<const uint8*>(colorBuffer) + y * colorPitch,
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
			stream << "frame,color_hash,depth_hash,gbuffer_hash,winner_hash,active_pixels,flush_ms,vertex_ms,raster_ms,pixel_ms,bin_ms,bin_bounds_ms,bin_count_ms,bin_prefix_ms,bin_scatter_ms,tile_size,tile_count,tile_jobs,relaxed_ssao,active_tiles,empty_tiles,bin_refs,batches,main_jobs,worker_jobs,active_workers,worker_busy_ms,worker_busy_max_ms,worker_busy_ratio,worker_balance,tile_busy_ratio,tile_worker_balance\n";
		stream << frameNumber << ',' << colorHash << ',' << depthHash << ',' << gbufferHash << ',' << winnerHash << ',' << activePixels << ','
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
			<< gEnv->profiler->getCount(ePe_MainThreadTaskNum) << ','
			<< workerJobs << ',' << activeWorkers << ',' << workerBusySum << ',' << workerBusyMax << ','
			<< workerBusyRatio << ',' << (workerBusyMean > 0.f ? workerBusyMax / workerBusyMean : 0.f) << ','
			<< gEnv->profiler->getTileBusyRatio() << ',' << gEnv->profiler->getTileWorkerBalance() << '\n';
	}
}

SrSoftRenderer::SrSoftRenderer(void):IRenderer(eRt_Software),
	m_window(NULL),
	m_rasterizer(NULL),
	m_renderState(0),
	m_cachedBuffer(NULL),
	m_bufferPitch(0),
	m_normalizeVertexBuffer(NULL),
	m_currShader(NULL),
	m_shaderConstants(NULL)
{
	m_textureStages.assign( SR_MAX_TEXTURE_STAGE_NUM , NULL );
	m_normalizeVBAllocSize = 0;

}

SrSoftRenderer::~SrSoftRenderer(void)
{
}

bool SrSoftRenderer::InitRenderer( HWND hWnd, int width, int height, int bpp )
{
	PERFORMANCEAPI_INSTRUMENT("Renderer.Init");
	m_window = hWnd;
	m_frameBuffer.assign(static_cast<size_t>(width) * height, 0);
	m_cachedBuffer = m_frameBuffer.data();
	m_bufferPitch = width * static_cast<int>(sizeof(uint32));

	m_normalizeVertexBuffer = (SrRendVertex*)_mm_malloc_custom( SR_NORMALIZE_VB_MAX_SIZE * sizeof(SrRendVertex), 16 );

	// 锟斤拷锟斤拷锟斤拷栅锟斤拷锟斤拷锟斤拷锟斤拷
	m_rasterizer = new SrRasterizer;
	m_rasterizer->Init(this);

	m_shaderConstants = (float4*)_mm_malloc_custom( eSC_ShaderConstantCount * sizeof(float4), 16 );
	memset(m_shaderConstants, 0, eSC_ShaderConstantCount * sizeof(float4));

	return true;
}

bool SrSoftRenderer::ShutdownRenderer()
{
	if (m_rasterizer)
	{
		if (g_context->IsFeatureEnable(eRFeature_TileBinning) && m_rasterizer->WaitForFrameAsync() &&
			m_rasterizer->IsFrameSurfaceReady())
		{
			WriteFrameHash(g_inFlightFrameNumber, m_cachedBuffer, m_bufferPitch);
			for (uint32 i = 0; i < g_inFlightNormalizedVBs.size(); ++i)
				delete g_inFlightNormalizedVBs[i];
			g_inFlightNormalizedVBs.clear();
			m_rasterizer->MarkFrameReadyForPresent();
		}
		m_rasterizer->PresentPendingFrame();
	}

	_mm_free_custom(m_normalizeVertexBuffer);
	m_normalizeVertexBuffer = 0;
	_mm_free_custom(m_shaderConstants);
	m_shaderConstants = 0;

	SAFE_DELETE( m_rasterizer );

	m_frameBuffer.clear();
	m_cachedBuffer = NULL;

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
	PERFORMANCEAPI_INSTRUMENT("Main.Swap");
	m_renderState |= eRS_Swaping;

	if (gEnv->present && !gEnv->present(m_window, m_cachedBuffer))
		return false;

	m_renderState &= ~eRS_Swaping;
	return true;
}

void SrSoftRenderer::BeginFrame()
{
	PERFORMANCEAPI_INSTRUMENT("Main.BeginFrame");
	m_renderState |= eRs_Rendering;
	m_frameCount++;

	ClearTextureStage();
}

void SrSoftRenderer::EndFrame()
{
	PERFORMANCEAPI_INSTRUMENT("Main.EndFrame");
	const bool tileBinning = g_context->IsFeatureEnable(eRFeature_TileBinning) != FALSE;
	if (tileBinning)
	{
		Swap();
		const bool completedFrame = m_rasterizer->WaitForFrameAsync();
		if (completedFrame && m_rasterizer->IsFrameSurfaceReady())
		{
			WriteFrameHash(g_inFlightFrameNumber, m_cachedBuffer, m_bufferPitch);
			for (uint32 i = 0; i < g_inFlightNormalizedVBs.size(); ++i)
				delete g_inFlightNormalizedVBs[i];
			g_inFlightNormalizedVBs.clear();
			m_rasterizer->MarkFrameReadyForPresent();
		}
		else if (completedFrame)
		{
			for (uint32 i = 0; i < g_inFlightNormalizedVBs.size(); ++i)
				delete g_inFlightNormalizedVBs[i];
			g_inFlightNormalizedVBs.clear();
		}

		// No render worker is active in this hand-off window. This keeps the
		// profiler reset free of cross-frame races.
		gEnv->profiler->Update();
		m_rasterizer->SubmitFrameAsync();
		g_inFlightFrameNumber = m_frameCount;
		g_inFlightNormalizedVBs.swap(m_normlizedVBs);

		// Present the completed frame, then hand the persistent CPU surface to the next frame.
		m_rasterizer->PresentPendingFrame();
		{
			PERFORMANCEAPI_INSTRUMENT("Main.LockFrameSurface");
			m_cachedBuffer = m_frameBuffer.data();
			m_bufferPitch = g_context->width * static_cast<int>(sizeof(uint32));
			m_rasterizer->SignalFrameSurface(true);
		}
	}
	else
	{
		gEnv->profiler->Update();
		m_cachedBuffer = m_frameBuffer.data();
		m_bufferPitch = g_context->width * static_cast<int>(sizeof(uint32));
		m_rasterizer->Flush();
		WriteFrameHash(m_frameCount, m_cachedBuffer, m_bufferPitch);
		for (uint32 i = 0; i < m_normlizedVBs.size(); ++i)
			delete m_normlizedVBs[i];
		m_normlizedVBs.clear();
		m_normalizeVBAllocSize = 0;
	}
	m_renderState &= ~eRs_Rendering;
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

void SrSoftRenderer::RegisterStaticShader(SrSwShader* shader, SrShader* binding)
{
	if (!shader || !binding)
		return;

	shader->m_bindShader = binding;
	m_swShaders.push_back(shader);
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

uint32 SrSoftRenderer::Tex2DGrad(float2& texcoord, const SrTexture* texture,
	uint32 packedTexcoordGradient) const
{
	if (!texture)
		return 0;

	const uint32 duBits = (packedTexcoordGradient & 0xffffu) << 16;
	const uint32 dvBits = packedTexcoordGradient & 0xffff0000u;
	float du;
	float dv;
	memcpy(&du, &duBits, sizeof(du));
	memcpy(&dv, &dvBits, sizeof(dv));
	const float footprintU = du * texture->getWidth();
	const float footprintV = dv * texture->getHeight();
	const float footprintSquared =
		footprintU * footprintU + footprintV * footprintV;
	uint32 mipLevel = 0;
	if (footprintSquared >= 2.f)
	{
		uint32 footprintBits;
		memcpy(&footprintBits, &footprintSquared, sizeof(footprintBits));
		const int exponent = static_cast<int>((footprintBits >> 23) & 0xffu) - 127;
		mipLevel = static_cast<uint32>((exponent + 1) >> 1);
	}
	const ESamplerFilter filter = g_context->IsFeatureEnable(eRFeature_LinearFiltering)
		? eSF_Linear : eSF_Nearest;
	return texture->Get(texcoord, filter, static_cast<float>(mipLevel));
}

const char* SrSoftRenderer::getName()
{
	return "SwCPU";
}
