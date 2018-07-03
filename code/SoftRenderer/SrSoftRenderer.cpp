/**
  @file SrRenderer.cpp
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#include "StdAfx.h"

#include "SrSoftRenderer.h"
#include "srBitmap.h"
#include "SrRasterizer.h"
#include "SrProfiler.h"
#include "SrSwShader.h"

#include "mmgr.h"
#include "BasicShaders.h"
#include "AdvanceShaders.h"

#define SR_NORMALIZE_VB_MAX_SIZE 1024 * 1024 * 10

SrSoftRenderer::SrSoftRenderer(void):IRenderer(eRt_Software),
	m_cachedBuffer(NULL),
	m_bufferPitch(0),
	m_renderState(0),
	m_normalizeVertexBuffer(NULL),
	m_shaderConstants(NULL),
	m_rasterizer(NULL)
{
	m_textureStages.assign( SR_MAX_TEXTURE_STAGE_NUM , NULL );
	m_normalizeVBAllocSize = 0;
}

SrSoftRenderer::~SrSoftRenderer(void)
{

}

bool SrSoftRenderer::InitRenderer(int width, int height, int bpp )
{
	// ������դ��������
	m_rasterizer = new SrRasterizer;
	m_rasterizer->Init(this);

	m_normalizeVertexBuffer = (SrRendVertex*)_mm_malloc_custom( SR_NORMALIZE_VB_MAX_SIZE * sizeof(SrRendVertex), 16 );

	m_shaderConstants = (float4*)_mm_malloc_custom( eSC_ShaderConstantCount * sizeof(float4), 16 );

	InnerInitShaders();

	return true;
}

bool SrSoftRenderer::ShutdownRenderer()
{
	_mm_free_custom(m_normalizeVertexBuffer);
	m_normalizeVertexBuffer = 0;
	_mm_free_custom(m_shaderConstants);
	m_shaderConstants = 0;

	SAFE_DELETE( m_rasterizer );
	return true;
}

bool SrSoftRenderer::HwClear()
{
	return true;
}

bool SrSoftRenderer::Swap()
{
	m_renderState |= eRS_Swaping;
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
	m_rasterizer->Flush();
 	m_textLines.clear();

	m_renderState &= ~eRs_Rendering;

	Swap();

	for (uint32 i=0; i < m_normlizedVBs.size(); ++i)
	{
		delete m_normlizedVBs[i];
	}
	m_normlizedVBs.clear();


	m_normalizeVBAllocSize = 0;
}

const uint8* SrSoftRenderer::getBuffer()
{
	return m_rasterizer->m_MemSBuffer->getBuffer();
}

int SrSoftRenderer::getBufferLength()
{
	int length = m_rasterizer->m_MemSBuffer->getWidth() * m_rasterizer->m_MemSBuffer->getHeight() * m_rasterizer->m_MemSBuffer->getBPP();
	return length;
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

uint32 SrSoftRenderer::Tex2D( const float2& texcoord, const SrTexture* texture  ) const
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

SrSwShader* LoadShader(const char* shaderName)
{
	std::vector<SrSwShader*> shaders;

	shaders.push_back(&g_FlatShadingShader);
	shaders.push_back(&g_PhongShadingShader);
	shaders.push_back(&g_GourandShadingShader);
	shaders.push_back(&g_PhongShadingWithNormalShader);

	shaders.push_back(&g_SkinSimShader);
	shaders.push_back(&g_FresnelNormalShader);
	shaders.push_back(&g_HairShader);

	for (uint32 i = 0; i < shaders.size(); ++i)
	{
		if (!strcmp(shaders[i]->getName(), shaderName))
		{
			return shaders[i];
		}
	}

	return NULL;
}

bool SrSoftRenderer::InnerInitShaders()
{
	m_swShaders.clear();

	//fnModuleInit fnCount = (fnModuleInit)(GetProcAddress( hDllHandle, "ModuleInit" ));
	//fnLoadShader fnLoad = (fnLoadShader)(GetProcAddress( hDllHandle, "LoadShader" ));

	//fnCount(gEnv);

	// add other shaders
	SrResourceManager* resMng = (SrResourceManager*)gEnv->resourceMgr;
	SrResourceLibrary::iterator it = resMng->m_shaderLibrary.begin();
	for (; it != resMng->m_shaderLibrary.end(); ++it)
	{
		SrSwShader* shader = LoadShader(it->second->getName());
		if (shader)
		{
			shader->m_bindShader = reinterpret_cast<SrShader*>(it->second);
			m_swShaders.push_back(shader);
		}

	}

	m_swShaders.push_back(&g_FlatShadingShader);
	m_swShaders.push_back(&g_PhongShadingShader);
	m_swShaders.push_back(&g_GourandShadingShader);
	m_swShaders.push_back(&g_PhongShadingWithNormalShader);

	m_swShaders.push_back(&g_SkinSimShader);
	m_swShaders.push_back(&g_FresnelNormalShader);
	m_swShaders.push_back(&g_HairShader);

	return true;
}

bool SrSoftRenderer::DrawScreenText( const char* str, int x,int y, uint32 size, uint32 color /*= SR_UICOLOR_HIGHLIGHT*/ )
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
