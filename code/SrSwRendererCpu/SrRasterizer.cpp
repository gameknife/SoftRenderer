/**
  @file SrRasterizer.cpp
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#include "StdAfx.h"
#include "SrRasterizer.h"
#include "SrProfiler.h"
#include "SrBitmap.h"
#include "SrMaterial.h"
#include "SrRasTaskDispatcher.h"
#include "SrRasTasks.h"
#include "SrSoftRenderer.h"
#include "SrFragmentBuffer.h"

#include "mmgr/mmgr.h"

SrFragmentBuffer* fBuffer = NULL;						/// fragment Buffer

SrRasterizer::SrRasterizer(void)
{	
	m_MemSBuffer = NULL;
	m_BackSBuffer = NULL;
}

/**
 *@brief 初始化光栅化器，初始化TaskDispatcher，申请rendertexture
 *@return void 
 */
void SrRasterizer::Init(SrSoftRenderer* renderer)
{
	fBuffer = new SrFragmentBuffer(g_context->width, g_context->height, renderer);
	gEnv->context->fBuffer = fBuffer;

	m_MemSBuffer = gEnv->resourceMgr->CreateRenderTexture( "$MemoryScreenBuffer" ,g_context->width, g_context->height, 4 );
	m_BackSBuffer = gEnv->resourceMgr->CreateRenderTexture( "$BackupScreenBuffer" ,g_context->width, g_context->height, 4 );

	m_rasTaskDispatcher = new SrRasTaskDispatcher;
	m_rasTaskDispatcher->Init();

	m_renderer = renderer;
}

SrRasterizer::~SrRasterizer(void)
{
	delete fBuffer;
	m_rasTaskDispatcher->Destroy();
	delete m_rasTaskDispatcher;
}

/**
 *@brief 绘制primitive
 *@return bool 绘制提交成功，返回true，否则false
 *@param SrPrimitve * primitive 需要绘制的primitive
 *@remark 函数内部负责将primitive的vb，ib拷贝到rendPrimitive内。\
 并缓存当前的textureStage，matrixStack，lightList。\
 为rendPrimitve建立shaderContext，以方便在shader内部访问。
 */
bool SrRasterizer::DrawPrimitive( SrPrimitve* primitive )
{
	if (!primitive)
	{
		return false;
	}

	float start = gEnv->timer->getRealTime();

	// 申请渲染primitive
	SrRendPrimitve* transformed = new SrRendPrimitve();

	// vb,ib申请
	float start1 = gEnv->timer->getRealTime();	
	transformed->vb = m_renderer->AllocateNormalizedVertexBuffer(primitive->vb->elementCount, true);
	transformed->ib = primitive->ib;
	gEnv->profiler->IncreaseTime(ePe_DrawCallAllocTime, gEnv->timer->getRealTime() - start1);

	// if recreate cached vb
	// Create Cached Vb
	if (!primitive->cachedVb)
	{
		SrVertexBuffer* cacheVB = m_renderer->AllocateNormalizedVertexBuffer( primitive->vb->elementCount );
		if (cacheVB)
		{
			// 填充渲染primitive
			for( uint32 i=0; i < primitive->vb->elementCount; ++i )
			{
				uint8* vbStart = primitive->vb->data;
				uint32 eSize = primitive->vb->elementSize;

				// 逐顶点填充
				memcpy( cacheVB->data + i * sizeof(SrRendVertex), vbStart + i * eSize, eSize);
			}
		}

		primitive->cachedVb = cacheVB;
	}


	memcpy(transformed->vb->data, primitive->cachedVb->data, primitive->cachedVb->elementSize * primitive->cachedVb->elementCount);
	
	// 填充ib
	//memcpy(transformed.ib->data, primitive->ib->data, primitive->ib->count * sizeof(uint32));
	
	// 填充shaderConstants
	transformed->shader = m_renderer->m_currShader;
	transformed->shaderConstants.matrixs = m_renderer->m_matrixStack; // matrixStack缓存
	transformed->shaderConstants.textureStage = m_renderer->m_textureStages; // 纹理stage缓存
	transformed->shaderConstants.lightList =gEnv->sceneMgr->m_lightList; // lightList缓存
	memcpy( transformed->shaderConstants.shaderConstants, m_renderer->m_shaderConstants, eSC_ShaderConstantCount * sizeof(float4) );
	transformed->shaderConstants.alphaTest = primitive->material->m_alphaTest;
	transformed->shaderConstants.culling = true;

	// 提交渲染primitive
	m_rendPrimitives.push_back( transformed );
	
	gEnv->profiler->IncreaseTime(ePe_DrawCallTime, gEnv->timer->getRealTime() - start);

	return true;
}

/**
 *@brief 光栅化器的工作入口

 *@return void 
 *@remark 每一帧末尾，对缓存的rendPrimitive进行处理。
 0.buffer的Clear操作
 1.并行的处理所有顶点,
 2.单线程执行各个primitive的光栅化,
 3.并行的处理所有得到的像素,
 4.后处理
 5.清空rendPrimitive等
 */
void SrRasterizer::Flush()
{
	gEnv->profiler->setBegin(ePe_FlushTime);
	gEnv->profiler->setBegin(ePe_ClearTime);

	//////////////////////////////////////////////////////////////////////////
	// 0. Clear操作

	// clear 像素索引
	fBuffer->GetPixelIndicesBuffer()->Clear();

	// Fragment Buffer 和 OutBaffer的Clear
	uint32* memBuffer = (uint32*)m_MemSBuffer->getBuffer();
	uint32* backBuffer = (uint32*)m_BackSBuffer->getBuffer();
	uint32* gpuBuffer = (uint32*)m_renderer->getBuffer();
	uint32* outBuffer = (uint32*)m_renderer->getBuffer();
	SrFragment* fragBuffer = fBuffer->fBuffer;

	// VIDEO MEM 2 SYSTEM MEM, VERY VERY VERY SLOW!!!
	//memcpy(m_prevSBuffer->getBuffer(), g_context->GetSBuffer(), 4 * g_context->width * g_context->height);
	// FAST, SO WE USE A MEM BACKBUFFER, ALL THINGS WRITE TO MEM BACKBUFFER, THEN JIT AA WRITE TO GPU BUFFER
	// 如果开启Jit AA，先写到memBuffer上
	if (g_context->IsFeatureEnable(eRFeature_JitAA) || g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
	{
		outBuffer = memBuffer;		
	}

	// clear the OutBuffer
	// 使用 SR_GREYSCALE_CLEARCOLOR 来清空缓存
	if (g_context->IsFeatureEnable(eRFeature_MThreadRendering))
	{
		// 多线程清空
		int quadsize = g_context->width * g_context->height;
		

		// 清空outBuffer
		uint8* dst = (uint8*)outBuffer;
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 0, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 1, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 2, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 3, quadsize, SR_GREYSCALE_CLEARCOLOR ));

		// 清空fragBuffer
		dst = (uint8*)fBuffer->zBuffer;
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 0, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 1, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 2, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 3, quadsize, 0 ));
		
		// 执行任务队列
		m_rasTaskDispatcher->FlushCoop();
		m_rasTaskDispatcher->Wait();
	}
	else
	{
		// 单线程清空
		int size = g_context->width * g_context->height;

		// 清空outBuffer
		memset(outBuffer, SR_GREYSCALE_CLEARCOLOR, 4 * size);
		// 清空fragBuffer
		memset(fragBuffer, 0, sizeof(SrFragment) * size);
	}
	gEnv->profiler->setEnd(ePe_ClearTime);


	//////////////////////////////////////////////////////////////////////////
	// 顶点处理
	// every vertex -> sshader -> every vertex
	gEnv->profiler->setBegin(ePe_VertexShaderTime);
	
	// 填充任务队列

	for ( std::list<SrRendPrimitve*>::iterator it = m_rendPrimitives.begin(); it != m_rendPrimitives.end(); ++it)
	{
		SrRendPrimitve& primitive = (**it);//m_rendPrimitives[i];
		uint32 vertexCount = 0;

		for (uint32 i = 0; i < primitive.vb->elementCount; i += VERTEX_TASK_BLOCK )
		{
			uint32 end =  i + VERTEX_TASK_BLOCK;
			if ( end > (primitive.vb->elementCount))
			{
				end = primitive.vb->elementCount;
			}
			m_rasTaskDispatcher->PushTask( new SrRasTask_Vertex(i, end, primitive.vb, &primitive) );
			gEnv->profiler->setIncrement(ePe_VertexCount, end - i);
		}
	}

// 	for ( uint32 i = 0; i < m_rendPrimitives.size(); ++i)
//  	{
// 		SrRendPrimitve& primitive = m_rendPrimitives[i];
// 		uint32 vertexCount = 0;
// 
// 		for (uint32 i = 0; i < primitive.vb->elementCount; i += VERTEX_TASK_BLOCK )
// 		{
// 			uint32 end =  i + VERTEX_TASK_BLOCK;
// 			if ( end > (primitive.vb->elementCount))
// 			{
// 				end = primitive.vb->elementCount;
// 			}
// 			m_rasTaskDispatcher->PushTask( new SrRasTask_Vertex(i, end, primitive.vb, &primitive) );
// 			gEnv->profiler->setIncrement(ePe_VertexCount, end - i);
// 		}
//  	}

	// 执行任务队列
	m_rasTaskDispatcher->FlushCoop();
	m_rasTaskDispatcher->Wait();

	gEnv->profiler->setEnd(ePe_VertexShaderTime);
	// 顶点处理结束
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// 光栅化
	// all vertex -> rasterization -> gbuffer
	gEnv->profiler->setBegin(ePe_RasterizeShaderTime);
#if 0
	for ( uint32 i = 0; i < m_rendPrimitives.size(); ++i)
	{
		gEnv->profiler->setIncrement(ePe_BatchCount);
		ProcessRasterizer( &(m_rendPrimitives[i]), g_context->GetFragBuffer() );
	}
#else
	for ( std::list<SrRendPrimitve*>::iterator it = m_rendPrimitives.begin(); it != m_rendPrimitives.end(); ++it)
	{
		

		gEnv->profiler->setIncrement(ePe_BatchCount);

		SrVertexBuffer* vb = (*it)->vb;
		SrIndexBuffer* ib = (*it)->ib;

		if (vb && ib)
		{
			uint32 triCount = ib->count / 3;

			for (uint32 j = 0; j < triCount; j += RASTERIZE_TASK_BLOCK )
			{
				uint32 end =  j + RASTERIZE_TASK_BLOCK;
				if ( end > triCount )
				{
					end = triCount;
				}
				m_rasTaskDispatcher->PushTask( new SrRasTask_Rasterize(&(**it), vb, ib, j, end ) );
			}
		}
	}
// 	for ( uint32 i = 0; i < m_rendPrimitives.size(); ++i)
// 	{
// 		gEnv->profiler->setIncrement(ePe_BatchCount);
// 
// 		SrVertexBuffer* vb = m_rendPrimitives[i].vb;
// 		SrIndexBuffer* ib = m_rendPrimitives[i].ib;
// 
// 		if (vb && ib)
// 		{
// 			uint32 triCount = ib->count / 3;
// 
// 			for (uint32 j = 0; j < triCount; j += RASTERIZE_TASK_BLOCK )
// 			{
// 				uint32 end =  j + RASTERIZE_TASK_BLOCK;
// 				if ( end > triCount )
// 				{
// 					end = triCount;
// 				}
// 				m_rasTaskDispatcher->PushTask( new SrRasTask_Rasterize(&(m_rendPrimitives[i]), vb, ib, j, end ) );
// 			}
// 		}
// 	}
	for ( std::list<SrRendPrimitve*>::iterator it = m_rendPrimitivesRHZ.begin(); it != m_rendPrimitivesRHZ.end(); ++it)
	{
		gEnv->profiler->setIncrement(ePe_BatchCount);

		SrVertexBuffer* vb = (*it)->vb;
		SrIndexBuffer* ib = (*it)->ib;

		if (vb && ib)
		{
			uint32 triCount = ib->count / 3;

			for (uint32 j = 0; j < triCount; j += RASTERIZE_TASK_BLOCK )
			{
				uint32 end =  j + RASTERIZE_TASK_BLOCK;
				if ( end > triCount )
				{
					end = triCount;
				}
				m_rasTaskDispatcher->PushTask( new SrRasTask_Rasterize(&(**it), vb, ib, j, end ) );
			}
		}
	}
// 	for ( uint32 i = 0; i < m_rendPrimitivesRHZ.size(); ++i)
// 	{
// 		gEnv->profiler->setIncrement(ePe_BatchCount);
// 
// 		SrVertexBuffer* vb = m_rendPrimitivesRHZ[i].vb;
// 		SrIndexBuffer* ib = m_rendPrimitivesRHZ[i].ib;
// 
// 		if (vb && ib)
// 		{
// 			uint32 triCount = ib->count / 3;
// 
// 			for (uint32 j = 0; j < triCount; j += RASTERIZE_TASK_BLOCK )
// 			{
// 				uint32 end =  j + RASTERIZE_TASK_BLOCK;
// 				if ( end > triCount )
// 				{
// 					end = triCount;
// 				}
// 				m_rasTaskDispatcher->PushTask( new SrRasTask_Rasterize(&(m_rendPrimitivesRHZ[i]), vb, ib, j, end ) );
// 			}
// 		}
// 	}

	// RHZ PRIMITIVE
	
	// 执行任务队列
	m_rasTaskDispatcher->FlushCoop();
	m_rasTaskDispatcher->Wait();
#endif
	gEnv->profiler->setEnd(ePe_RasterizeShaderTime);
	// 光栅化结束
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 像素处理
	// every gbuffer pixel -> pshader -> every screenbuffer pixel	
	gEnv->profiler->setBegin(ePe_PixelShaderTime);

	// 任务统计
	for ( uint32 i=0; i < g_context->width * g_context->height; ++i)
	{
		if(fBuffer->zBuffer[i] < 0.f)
		{
			fBuffer->GetPixelIndicesBuffer()->push_back(i);
		}
	}


	// 任务填充
	uint32 size = fBuffer->GetPixelIndicesBuffer()->size();
	for ( uint32 i = 0 ; i < size; i += PIXEL_TASK_BLOCK)
	{
		uint32 end =  i + PIXEL_TASK_BLOCK;
		if ( end > size)
		{
			end = size;
		}
		m_rasTaskDispatcher->PushTask( new SrRasTask_Pixel( i, end, fBuffer->GetPixelIndicesBuffer()->data, fBuffer->fBuffer, outBuffer ) );
		gEnv->profiler->setIncrement(ePe_PixelCount, end - i);
	}

	// 执行任务队列
	m_rasTaskDispatcher->FlushCoop();
	m_rasTaskDispatcher->Wait();
	gEnv->profiler->setEnd(ePe_PixelShaderTime);
	// 像素处理结束
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// line helper draw

	// FIXME

// 	for (uint32 i=0; i < m_rendDynamicVertex.size(); i+=2)
// 	{
// 		float4 line0 = gEnv->renderer->GetMatrix(eMd_WorldViewProj) * m_rendDynamicVertex[i];
// 		float4 line1 = gEnv->renderer->GetMatrix(eMd_WorldViewProj) * m_rendDynamicVertex[i+1];
// 		
// 		// trans to screen space
// 		if (line0.w < .5f)
// 		{
// 			line0.w = .5f;
// 		}
// 		if (line1.w < .5f)
// 		{
// 			line1.w = .5f;
// 		}
// 
// 		line0.xyz /= line0.w;
// 		line1.xyz /= line1.w;
// 
// 		line0.x = ((-line0.x * .5f + 0.5f) * g_context->width);
// 		line0.y = ((-line0.y * .5f + 0.5f) * g_context->height);
// 		line1.x = ((-line1.x * .5f + 0.5f) * g_context->width);
// 		line1.y = ((-line1.y * .5f + 0.5f) * g_context->height);
// 
// 		Draw_Clip_Line( (int)line0.x, (int)line0.y, (int)line1.x, (int)line1.y, 0xffffffff, outBuffer, g_context->width );
// 	}



	//////////////////////////////////////////////////////////////////////////
	// 后期处理
	gEnv->profiler->setBegin(ePe_PostProcessTime);

	// SSAO

	// Depth of Field

	// jit AA
	if (g_context->IsFeatureEnable(eRFeature_JitAA) || g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
	{
		int quadsize = g_context->width * g_context->height / 4;

		m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( 0,				quadsize,		memBuffer, backBuffer, gpuBuffer ) );
		m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( quadsize,		quadsize * 2,	memBuffer, backBuffer, gpuBuffer ) );
		m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( quadsize * 2,	quadsize * 3,	memBuffer, backBuffer, gpuBuffer ) );
		m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( quadsize * 3,	quadsize * 4,	memBuffer, backBuffer, gpuBuffer ) );

		m_rasTaskDispatcher->FlushCoop();
		m_rasTaskDispatcher->Wait();

		memcpy( backBuffer, memBuffer, 4 * g_context->width * g_context->height);		
	}
	
	// 后期处理结束
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// 清理工作

	// 清除渲染primitives
	for(std::list<SrRendPrimitve*>::iterator it = m_rendPrimitives.begin(); it != m_rendPrimitives.end(); ++it)
	{
		delete (*it);
	}
	m_rendPrimitives.clear();
	m_rendPrimitivesRHZ.clear();
	m_rendDynamicVertex.clear();

	//////////////////////////////////////////////////////////////////////////
	gEnv->profiler->setEnd(ePe_PostProcessTime);
	gEnv->profiler->setEnd(ePe_FlushTime);
}

/**
 *@brief 光栅化一个rendPrimitive
 *@return void 
 *@param SrRendPrimitves * in_primitive 输入的rendPrimitive 
 *@param SrFragmentBuffer * out_gBuffer 输出的fragBuffer
 */
void SrRasterizer::ProcessRasterizer( SrRendPrimitve* in_primitive, SrFragment* out_gBuffer )
{
	SrVertexBuffer* vb = in_primitive->vb;
	SrIndexBuffer* ib = in_primitive->ib;

	// 用ib索引，构造三角形进行光栅化
	if (vb && ib)
	{
		uint32 triCount = ib->count / 3;
		// tiled
		uint32 startIdx = 0;
		uint32 processSize = triCount;

		for ( uint32 i=startIdx; i < processSize; ++i )
		{
			SrRastTriangle tri;
			tri.p[0] = *(SrRendVertex*)(vb->data + ib->data[i * 3 + 0] * vb->elementSize);
			tri.p[1] = *(SrRendVertex*)(vb->data + ib->data[i * 3 + 1] * vb->elementSize);
			tri.p[2] = *(SrRendVertex*)(vb->data + ib->data[i * 3 + 2] * vb->elementSize);
			tri.primitive = in_primitive;

			// 提交三角形进行裁剪
			RasterizeTriangle_Clip(tri, g_context->viewport.n, g_context->viewport.f);
		}
	}
}

bool SrRasterizer::DrawLine( const float3& from, const float3& to )
{
	m_rendDynamicVertex.push_back( float4(from, 1.f) );
	m_rendDynamicVertex.push_back( float4(to, 1.f) );
	return true;
}

bool SrRasterizer::DrawRHZPrimitive( SrRendPrimitve& rendPrimitive )
{
	// 填充shaderConstants
// 	rendPrimitive.shaderConstants.matrixs = m_renderer->m_matrixStack; // matrixStack缓存
// 	rendPrimitive.shaderConstants.textureStage = m_renderer->m_textureStages; // 纹理stage缓存
// 	rendPrimitive.shaderConstants.lightList = gEnv->sceneMgr->m_lightList; // lightList缓存
// 	rendPrimitive.shaderConstants.cBuffer = rendPrimitive.material->m_cbuffer; // 材质的cBuffer缓存
// 	rendPrimitive.shaderConstants.alphaTest = true;
// 	rendPrimitive.shaderConstants.culling = false;

	m_rendPrimitivesRHZ.push_back(&rendPrimitive);
	return true;
}
void SrRendPrimitve::operator delete(void *memoryToBeDeallocated)
{
	_mm_free_custom(memoryToBeDeallocated);
}
void * SrRendPrimitve::operator new(size_t size)
{

	return _mm_malloc_custom(size, 16);
}