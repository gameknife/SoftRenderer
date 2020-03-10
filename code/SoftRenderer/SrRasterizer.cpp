/**
  @file SrRasterizer.cpp
  
  @author yikaiming

 */

#include "stdafx.h"
#include "SrRasterizer.h"
#include "SrProfiler.h"
#include "SrBitmap.h"
#include "SrMaterial.h"
#include "SrRasTaskDispatcher.h"
#include "SrRasTasks.h"
#include "SrSoftRenderer.h"
#include "SrFragmentBuffer.h"


SrFragmentBuffer* fBuffer = NULL;						/// fragment Buffer

SrRasterizer::SrRasterizer(void)
{	
	m_MemSBuffer = NULL;
	m_BackS1Buffer = NULL;
	m_BackS2Buffer = NULL;
}

void SrRasterizer::Init(SrSoftRenderer* renderer)
{
	fBuffer = new SrFragmentBuffer(g_context->width, g_context->height);
	gEnv->context->fBuffer = fBuffer;

	m_MemSBuffer = gEnv->resourceMgr->CreateRenderTexture( "$MemoryScreenBuffer" ,g_context->width, g_context->height, 4 );
	m_BackS1Buffer = gEnv->resourceMgr->CreateRenderTexture( "$BackupScreen1Buffer" ,g_context->width, g_context->height, 4 );
	m_BackS2Buffer = gEnv->resourceMgr->CreateRenderTexture("$BackupScreen2Buffer", g_context->width, g_context->height, 4);

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

bool SrRasterizer::DrawPrimitive( SrPrimitve* primitive )
{
	if (!primitive)
	{
		return false;
	}

	float start = gEnv->timer->getRealTime();

	// ������Ⱦprimitive
	SrRendPrimitve* transformed = new SrRendPrimitve();

	// vb,ib����
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
			// �����Ⱦprimitive
			for( uint32 i=0; i < primitive->vb->elementCount; ++i )
			{
				uint8* vbStart = primitive->vb->data;
				uint32 eSize = primitive->vb->elementSize;

				// �𶥵����
				memcpy( cacheVB->data + i * sizeof(SrRendVertex), vbStart + i * eSize, eSize);
			}
		}

		primitive->cachedVb = cacheVB;
	}


	memcpy(transformed->vb->data, primitive->cachedVb->data, primitive->cachedVb->elementSize * primitive->cachedVb->elementCount);
	//memcpy(transformed.ib->data, primitive->ib->data, primitive->ib->count * sizeof(uint32));
	
	// shaderConstants
	transformed->shader = m_renderer->m_currShader;
	transformed->shaderConstants.matrixs = m_renderer->m_matrixStack; // matrixStack
	transformed->shaderConstants.textureStage = m_renderer->m_textureStages; // stage
	transformed->shaderConstants.lightList =gEnv->sceneMgr->m_lightList; // lightList
	memcpy( transformed->shaderConstants.shaderConstants, m_renderer->m_shaderConstants, eSC_ShaderConstantCount * sizeof(float4) );
	transformed->shaderConstants.alphaTest = primitive->material->m_alphaTest;
	transformed->shaderConstants.culling = true;

	// primitive
	m_rendPrimitives.push_back( transformed );
	
	gEnv->profiler->IncreaseTime(ePe_DrawCallTime, gEnv->timer->getRealTime() - start);

	return true;
}

void SrRasterizer::Flush()
{
	gEnv->profiler->setBegin(ePe_FlushTime);
	gEnv->profiler->setBegin(ePe_ClearTime);

	//////////////////////////////////////////////////////////////////////////
	// 0. Clear

	fBuffer->GetPixelIndicesBuffer()->Clear();

	// Fragment Buffer OutBaffer Clear
	uint32* memBuffer = (uint32*)m_MemSBuffer->getBuffer();



	uint32* aaBufferWrite = (uint32*)m_BackS1Buffer->getBuffer();
	uint32* aaBufferRead = (uint32*)m_BackS2Buffer->getBuffer();

	if (gEnv->timer->getFramecount() % 2 == 0)
	{
		aaBufferWrite = (uint32*)m_BackS2Buffer->getBuffer();
		aaBufferRead = (uint32*)m_BackS1Buffer->getBuffer();
	}

	//uint32* gpuBuffer = (uint32*)m_renderer->getBuffer();
	uint32* outBuffer = memBuffer;// (uint32*)m_renderer->getBuffer();


	if (g_context->IsFeatureEnable(eRFeature_JitAA) || g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
	{
		outBuffer = aaBufferWrite;
	}

	{
		int quadsize = g_context->width * g_context->height;
		

		// outBuffer
		uint8* dst = (uint8*)outBuffer;
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 0, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 1, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 2, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 3, quadsize, SR_GREYSCALE_CLEARCOLOR ));

		// fragBuffer
		dst = (uint8*)fBuffer->zBuffer;
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 0, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 1, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 2, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 3, quadsize, 0 ));
		
		// 
		m_rasTaskDispatcher->Flush();
		m_rasTaskDispatcher->Wait();
	}
	gEnv->profiler->setEnd(ePe_ClearTime);


	//////////////////////////////////////////////////////////////////////////
	//
	// every vertex -> sshader -> every vertex
	gEnv->profiler->setBegin(ePe_VertexShaderTime);
	
	for ( std::list<SrRendPrimitve*>::iterator it = m_rendPrimitives.begin(); it != m_rendPrimitives.end(); ++it)
	{
		SrRendPrimitve& primitive = (**it);

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

	m_rasTaskDispatcher->Flush();
	m_rasTaskDispatcher->Wait();

	gEnv->profiler->setEnd(ePe_VertexShaderTime);

	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
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

	// RHZ PRIMITIVE
	
	m_rasTaskDispatcher->Flush();
	m_rasTaskDispatcher->Wait();
#endif
	gEnv->profiler->setEnd(ePe_RasterizeShaderTime);

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// PS 阶段
	// every gbuffer pixel -> pshader -> every screenbuffer pixel	
	gEnv->profiler->setBegin(ePe_PixelShaderTime);

	// 剔除不需要渲染的
	
	for ( uint32 i=0; i < g_context->width * g_context->height; ++i)
	{
		if(fBuffer->zBuffer[i] < 0.f)
		{
			fBuffer->GetPixelIndicesBuffer()->push_back(i);
		}
	}


	
	// 任务分配
	{
		uint32 size = fBuffer->GetPixelIndicesBuffer()->size();

		// Task栈上分配 854 x 480 / 512 + 1 = 801
		// task结构需要精简
		// 栈上有问题，其他核访问不到
		static SrRasTask_Pixel tmpTasks[1800];
		uint32 taskAddress = 0;

		for (uint32 i = 0; i < size; i += PIXEL_TASK_BLOCK)
		{
			uint32 end = i + PIXEL_TASK_BLOCK;
			if (end > size)
			{
				end = size;
			}
			auto taskptr = &( tmpTasks[taskAddress++] );
			taskptr->m_indexStart = i;
			taskptr->m_indexEnd = end;
			taskptr->m_indexBuffer = fBuffer->GetPixelIndicesBuffer()->data;
			taskptr->m_gBuffer = fBuffer->fBuffer;
			taskptr->m_oBuffer = outBuffer;
			//m_rasTaskDispatcher->PrePushTask(new SrRasTask_Pixel(i, end, fBuffer->GetPixelIndicesBuffer()->data, fBuffer->fBuffer, outBuffer));
			m_rasTaskDispatcher->PushTask(taskptr);
			gEnv->profiler->setIncrement(ePe_PixelCount, end - i);
		}

		// 优化一下，直接平均预分配到每个线程


		m_rasTaskDispatcher->Flush();
		m_rasTaskDispatcher->Wait();
	}

	gEnv->profiler->setEnd(ePe_PixelShaderTime);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 
	gEnv->profiler->setBegin(ePe_PostProcessTime);

	// jit AA
	if (g_context->IsFeatureEnable(eRFeature_JitAA) || g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
	{
		int quadsize = g_context->width * g_context->height / 4;

		 m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( 0,				quadsize,		aaBufferWrite, aaBufferRead, memBuffer) );
		 m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( quadsize,		quadsize * 2,	aaBufferWrite, aaBufferRead, memBuffer) );
		 m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( quadsize * 2,	quadsize * 3,	aaBufferWrite, aaBufferRead, memBuffer) );
		 m_rasTaskDispatcher->PushTask( new SrRasTask_JitAA( quadsize * 3,	quadsize * 4,	aaBufferWrite, aaBufferRead, memBuffer) );
  
		 m_rasTaskDispatcher->Flush();
		 m_rasTaskDispatcher->Wait();

		//memcpy( backBuffer, memBuffer, 4 * g_context->width * g_context->height);
	}
	
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// 
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

void SrRasterizer::ProcessRasterizer( SrRendPrimitve* in_primitive, SrFragment* out_gBuffer )
{
	SrVertexBuffer* vb = in_primitive->vb;
	SrIndexBuffer* ib = in_primitive->ib;

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

			RasterizeTriangle_Clip(tri, g_context->viewport.n, g_context->viewport.f);
		}
	}
}

bool SrRasterizer::DrawLine( const float3& from, const float3& to )
{
	m_rendDynamicVertex.push_back( float4::make(from, 1.f) );
	m_rendDynamicVertex.push_back( float4::make(to, 1.f) );
	return true;
}

bool SrRasterizer::DrawRHZPrimitive( SrRendPrimitve& rendPrimitive )
{
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