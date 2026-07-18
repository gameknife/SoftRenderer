/**
  @file SrRasterizer.cpp

  @author yikaiming

  ������־ history
  ver:1.0

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
#include "SrPerformance.h"

#include "mmgr/mmgr.h"

SrFragmentBuffer* fBuffer = NULL;						/// fragment Buffer
static const uint32 SR_FRAME_VERTEX_CAPACITY = 1024 * 1024 * 10;

SrRasterizer::SrRasterizer(void)
	: m_rendPrimitiveUsed(0), m_buildPrimitiveUsed(0), m_renderVertexBuffer(NULL),
	  m_frameSurfaceReady(false), m_presentPending(false), m_surfaceReadyForRender(false),
	  m_frameRequested(false), m_frameComplete(false), m_frameInFlight(false), m_stopFrameThread(false)
{
	m_MemSBuffer = NULL;
	m_BackSBuffer = NULL;
}

/**
 *@brief ��ʼ����դ��������ʼ��TaskDispatcher������rendertexture
 *@return void
 */
void SrRasterizer::Init(SrSoftRenderer* renderer)
{
	fBuffer = new SrFragmentBuffer(g_context->width, g_context->height);
	gEnv->context->fBuffer = fBuffer;

	m_MemSBuffer = gEnv->resourceMgr->CreateRenderTexture( "$MemoryScreenBuffer" ,g_context->width, g_context->height, 4 );
	m_BackSBuffer = gEnv->resourceMgr->CreateRenderTexture( "$BackupScreenBuffer" ,g_context->width, g_context->height, 4 );
	const size_t screenBytes = static_cast<size_t>(g_context->width) * g_context->height * sizeof(uint32);
	memset(const_cast<uint8*>(m_MemSBuffer->getBuffer()), SR_GREYSCALE_CLEARCOLOR, screenBytes);
	memset(const_cast<uint8*>(m_BackSBuffer->getBuffer()), SR_GREYSCALE_CLEARCOLOR, screenBytes);

	m_rasTaskDispatcher = new SrRasTaskDispatcher;
	m_rasTaskDispatcher->Init();

	m_renderer = renderer;
	m_renderVertexBuffer = static_cast<SrRendVertex*>(_mm_malloc_custom(
		SR_FRAME_VERTEX_CAPACITY * sizeof(SrRendVertex), 16));
	assert(m_renderVertexBuffer != NULL);
	m_frameThread = std::thread(&SrRasterizer::FrameThreadMain, this);
}

SrRasterizer::~SrRasterizer(void)
{
	StopFrameThread();
	delete fBuffer;
	m_rasTaskDispatcher->Destroy();
	delete m_rasTaskDispatcher;
	for (uint32 i = 0; i < m_rendPrimitivePool.size(); ++i)
		delete m_rendPrimitivePool[i];
	for (uint32 i = 0; i < m_buildPrimitivePool.size(); ++i)
		delete m_buildPrimitivePool[i];
	_mm_free_custom(m_renderVertexBuffer);
}

void SrRasterizer::FrameThreadMain()
{
	SrSetCurrentThreadName("Renderer Coordinator");
	for (;;)
	{
		std::unique_lock<std::mutex> lock(m_frameMutex);
		m_frameRequestCondition.wait(lock, [this] { return m_frameRequested || m_stopFrameThread; });
		if (m_stopFrameThread)
			break;
		m_frameRequested = false;
		lock.unlock();
		{
			PERFORMANCEAPI_INSTRUMENT("Renderer.CoordinatorFrame");
			Flush();
		}
		lock.lock();
		m_frameComplete = true;
		lock.unlock();
		m_frameCompleteCondition.notify_one();
	}
}

void SrRasterizer::WaitForFrameSurface(void* context)
{
	PERFORMANCEAPI_INSTRUMENT("Renderer.WaitForSurface");
	SrRasterizer* rasterizer = static_cast<SrRasterizer*>(context);
	std::unique_lock<std::mutex> lock(rasterizer->m_frameMutex);
	rasterizer->m_surfaceReadyCondition.wait(lock, [rasterizer] {
		return rasterizer->m_surfaceReadyForRender || rasterizer->m_stopFrameThread;
	});
}

void SrRasterizer::SubmitFrameAsync()
{
	PERFORMANCEAPI_INSTRUMENT("Main.SubmitFrame");
	assert(!m_frameInFlight);
	m_rendPrimitives.swap(m_buildRendPrimitives);
	m_rendPrimitivesRHZ.swap(m_buildRendPrimitivesRHZ);
	m_rendDynamicVertex.swap(m_buildDynamicVertex);
	m_rendPrimitivePool.swap(m_buildPrimitivePool);
	m_rendPrimitiveUsed = m_buildPrimitiveUsed;
	m_buildPrimitiveUsed = 0;
	std::swap(m_renderVertexBuffer, m_renderer->m_normalizeVertexBuffer);
	m_renderer->m_normalizeVBAllocSize = 0;

	{
		std::lock_guard<std::mutex> lock(m_frameMutex);
		m_frameComplete = false;
		m_surfaceReadyForRender = false;
		m_frameInFlight = true;
		m_frameRequested = true;
	}
	m_frameRequestCondition.notify_one();
}

bool SrRasterizer::WaitForFrameAsync()
{
	PERFORMANCEAPI_INSTRUMENT("Main.WaitForPreviousFrame");
	std::unique_lock<std::mutex> lock(m_frameMutex);
	if (!m_frameInFlight)
		return false;
	m_frameCompleteCondition.wait(lock, [this] { return m_frameComplete; });
	m_frameInFlight = false;
	return true;
}

void SrRasterizer::SignalFrameSurface(bool ready)
{
	{
		std::lock_guard<std::mutex> lock(m_frameMutex);
		m_frameSurfaceReady = ready;
		m_surfaceReadyForRender = true;
	}
	m_surfaceReadyCondition.notify_one();
}

void SrRasterizer::StopFrameThread()
{
	if (!m_frameThread.joinable())
		return;
	WaitForFrameAsync();
	{
		std::lock_guard<std::mutex> lock(m_frameMutex);
		m_stopFrameThread = true;
		m_surfaceReadyForRender = true;
	}
	m_frameRequestCondition.notify_one();
	m_surfaceReadyCondition.notify_all();
	m_frameThread.join();
}

void SrRasterizer::PresentPendingFrame()
{
	if (!m_presentPending)
		return;
	m_presentPending = false;
}

/**
 *@brief ����primitive
 *@return bool �����ύ�ɹ�������true������false
 *@param SrPrimitve * primitive ��Ҫ���Ƶ�primitive
 *@remark �����ڲ�����primitive��vb��ib������rendPrimitive�ڡ�\
 �����浱ǰ��textureStage��matrixStack��lightList��\
 ΪrendPrimitve����shaderContext���Է�����shader�ڲ����ʡ�
 */
bool SrRasterizer::DrawPrimitive( SrPrimitve* primitive )
{
	PERFORMANCEAPI_INSTRUMENT("Main.DrawPrimitive");
	if (!primitive)
	{
		return false;
	}

	float start = gEnv->timer->getRealTime();
	const bool asyncFrame = g_context->IsFeatureEnable(eRFeature_TileBinning) != FALSE;
	std::vector<SrRendPrimitve*>& primitivePool = asyncFrame ? m_buildPrimitivePool : m_rendPrimitivePool;
	uint32& primitiveUsed = asyncFrame ? m_buildPrimitiveUsed : m_rendPrimitiveUsed;
	std::list<SrRendPrimitve*>& drawList = asyncFrame ? m_buildRendPrimitives : m_rendPrimitives;

	SrRendPrimitve* transformed = NULL;
	if (primitiveUsed < primitivePool.size())
		transformed = primitivePool[primitiveUsed];
	else
	{
		transformed = new SrRendPrimitve();
		primitivePool.push_back(transformed);
	}
	++primitiveUsed;

	// vb,ib����
	float start1 = gEnv->timer->getRealTime();
	transformed->sourceVb = NULL;
	transformed->vb = &transformed->frameVb;
	transformed->vb->data = reinterpret_cast<uint8*>(m_renderer->m_normalizeVertexBuffer + m_renderer->m_normalizeVBAllocSize);
	transformed->vb->elementCount = primitive->vb->elementCount;
	transformed->vb->elementSize = sizeof(SrRendVertex);
	m_renderer->m_normalizeVBAllocSize += primitive->vb->elementCount;
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


	if (!primitive->cachedVb)
		return false;
	transformed->sourceVb = primitive->cachedVb;
	// The tile path reads cachedVb directly and writes VS results out of place.
	// Keep the copy for the legacy in-place vertex task until that path is retired.
	if (!g_context->IsFeatureEnable(eRFeature_TileBinning))
		memcpy(transformed->vb->data, transformed->sourceVb->data,
			transformed->sourceVb->elementSize * transformed->sourceVb->elementCount);

	// ���ib
	//memcpy(transformed.ib->data, primitive->ib->data, primitive->ib->count * sizeof(uint32));

	// ���shaderConstants
	transformed->shader = m_renderer->m_currShader;
	transformed->shaderConstants.matrixs = m_renderer->m_matrixStack; // matrixStack����
	transformed->shaderConstants.textureStage = m_renderer->m_textureStages; // ����stage����
	transformed->shaderConstants.lightList =gEnv->sceneMgr->m_lightList; // lightList����
	memcpy( transformed->shaderConstants.shaderConstants, m_renderer->m_shaderConstants, eSC_ShaderConstantCount * sizeof(float4) );
	transformed->shaderConstants.alphaTest = primitive->material->m_alphaTest;
	transformed->shaderConstants.culling = true;
	transformed->drawSequence = static_cast<uint32>(drawList.size());
	transformed->frameNumber = m_renderer->getFrameCount();
	transformed->zNear = g_context->viewport.n;
	transformed->zFar = g_context->viewport.f;

	// �ύ��Ⱦprimitive
	drawList.push_back( transformed );

	gEnv->profiler->IncreaseTime(ePe_DrawCallTime, gEnv->timer->getRealTime() - start);

	return true;
}

/**
 *@brief ��դ�����Ĺ������

 *@return void
 *@remark ÿһ֡ĩβ���Ի����rendPrimitive���д����
 0.buffer��Clear����
 1.���еĴ������ж���,
 2.���߳�ִ�и���primitive�Ĺ�դ��,
 3.���еĴ������еõ�������,
 4.����
 5.���rendPrimitive��
 */
void SrRasterizer::Flush()
{
	PERFORMANCEAPI_INSTRUMENT("Render.Flush");
	gEnv->profiler->setBegin(ePe_FlushTime);
	gEnv->profiler->setBegin(ePe_ClearTime);

	//////////////////////////////////////////////////////////////////////////
	// 0. Clear����

	// clear ��������
	fBuffer->GetPixelIndicesBuffer()->Clear();

	if (g_context->IsFeatureEnable(eRFeature_TileBinning))
	{
		FlushTileBinning();
		gEnv->profiler->setBegin(ePe_PostProcessTime);
		m_rendPrimitives.clear();
		m_rendPrimitivesRHZ.clear();
		m_rendDynamicVertex.clear();
		m_rendPrimitiveUsed = 0;
		gEnv->profiler->setEnd(ePe_PostProcessTime);
		gEnv->profiler->setEnd(ePe_FlushTime);
		return;
	}

	// Fragment Buffer �� OutBaffer��Clear
	uint32* memBuffer = (uint32*)m_MemSBuffer->getBuffer();
	uint32* backBuffer = (uint32*)m_BackSBuffer->getBuffer();
	uint32* gpuBuffer = (uint32*)m_renderer->getBuffer();
	uint32* outBuffer = (uint32*)m_renderer->getBuffer();
	SrFragment* fragBuffer = fBuffer->fBuffer;

	// VIDEO MEM 2 SYSTEM MEM, VERY VERY VERY SLOW!!!
	//memcpy(m_prevSBuffer->getBuffer(), g_context->GetSBuffer(), 4 * g_context->width * g_context->height);
	// FAST, SO WE USE A MEM BACKBUFFER, ALL THINGS WRITE TO MEM BACKBUFFER, THEN JIT AA WRITE TO GPU BUFFER
	// �������Jit AA����д��memBuffer��
	if (g_context->IsFeatureEnable(eRFeature_JitAA) || g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
	{
		outBuffer = memBuffer;
	}

	// clear the OutBuffer
	// ʹ�� SR_GREYSCALE_CLEARCOLOR ����ջ���
	if (g_context->IsFeatureEnable(eRFeature_MThreadRendering))
	{
		// ���߳����
		int quadsize = g_context->width * g_context->height;


		// ���outBuffer
		uint8* dst = (uint8*)outBuffer;
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 0, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 1, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 2, quadsize, SR_GREYSCALE_CLEARCOLOR ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 3, quadsize, SR_GREYSCALE_CLEARCOLOR ));

		// ���fragBuffer
		dst = (uint8*)fBuffer->zBuffer;
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 0, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 1, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 2, quadsize, 0 ));
		m_rasTaskDispatcher->PushTask( new SrRasTask_Clear( dst + quadsize * 3, quadsize, 0 ));

		// ִ���������
		m_rasTaskDispatcher->FlushCoop();
		m_rasTaskDispatcher->Wait();
	}
	else
	{
		// ���߳����
		int size = g_context->width * g_context->height;

		// ���outBuffer
		memset(outBuffer, SR_GREYSCALE_CLEARCOLOR, 4 * size);
		// ���fragBuffer
		memset(fragBuffer, 0, sizeof(SrFragment) * size);
		memset(fBuffer->zBuffer, 0, sizeof(float) * size);
	}
	gEnv->profiler->setEnd(ePe_ClearTime);


	//////////////////////////////////////////////////////////////////////////
	// ���㴦��
	// every vertex -> sshader -> every vertex
	gEnv->profiler->setBegin(ePe_VertexShaderTime);

	// ����������

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

	// ִ���������
	m_rasTaskDispatcher->FlushCoop();
	m_rasTaskDispatcher->Wait();

	gEnv->profiler->setEnd(ePe_VertexShaderTime);
	// ���㴦�����
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// ��դ��
	// all vertex -> rasterization -> gbuffer
	gEnv->profiler->setBegin(ePe_RasterizeShaderTime);
#if 0
	for ( uint32 i = 0; i < m_rendPrimitives.size(); ++i)
	{
		gEnv->profiler->setIncrement(ePe_BatchCount);
		ProcessRasterizer( &(m_rendPrimitives[i]), g_context->GetFragBuffer() );
	}
#else
	int submittedTriangleCount = 0;
	for ( std::list<SrRendPrimitve*>::iterator it = m_rendPrimitives.begin(); it != m_rendPrimitives.end(); ++it)
	{


		gEnv->profiler->setIncrement(ePe_BatchCount);

		SrVertexBuffer* vb = (*it)->vb;
		SrIndexBuffer* ib = (*it)->ib;

		if (vb && ib)
		{
			uint32 triCount = ib->count / 3;
			submittedTriangleCount += static_cast<int>(triCount);

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
			submittedTriangleCount += static_cast<int>(triCount);

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

	gEnv->profiler->setIncrement(ePe_TriangleCount, submittedTriangleCount);

	// RHZ PRIMITIVE

	// ִ���������
	m_rasTaskDispatcher->FlushCoop();
	m_rasTaskDispatcher->Wait();
#endif
	gEnv->profiler->setEnd(ePe_RasterizeShaderTime);
	// ��դ������
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// ���ش���
	// every gbuffer pixel -> pshader -> every screenbuffer pixel
	gEnv->profiler->setBegin(ePe_PixelShaderTime);

	// ����ͳ��
	const uint32 pixelCount = static_cast<uint32>(g_context->width) * static_cast<uint32>(g_context->height);
	for (uint32 i = 0; i < pixelCount; ++i)
	{
		if(fBuffer->zBuffer[i] < 0.f)
		{
			fBuffer->GetPixelIndicesBuffer()->push_back(i);
		}
	}


	// �������
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

	// ִ���������
	m_rasTaskDispatcher->FlushCoop();
	m_rasTaskDispatcher->Wait();
	gEnv->profiler->setEnd(ePe_PixelShaderTime);
	// ���ش������
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
	// ���ڴ���
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

	// ���ڴ������
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// �������

	// �����Ⱦprimitives
	m_rendPrimitives.clear();
	m_rendPrimitivesRHZ.clear();
	m_rendDynamicVertex.clear();
	m_rendPrimitiveUsed = 0;

	//////////////////////////////////////////////////////////////////////////
	gEnv->profiler->setEnd(ePe_PostProcessTime);
	gEnv->profiler->setEnd(ePe_FlushTime);
}

/**
 *@brief ��դ��һ��rendPrimitive
 *@return void
 *@param SrRendPrimitves * in_primitive �����rendPrimitive
 *@param SrFragmentBuffer * out_gBuffer �����fragBuffer
 */
void SrRasterizer::ProcessRasterizer( SrRendPrimitve* in_primitive, SrFragment* out_gBuffer )
{
	SrVertexBuffer* vb = in_primitive->vb;
	SrIndexBuffer* ib = in_primitive->ib;

	// ��ib���������������ν��й�դ��
	if (vb && ib)
	{
		uint32 triCount = ib->count / 3;
		gEnv->profiler->setIncrement(ePe_TriangleCount, static_cast<int>(triCount));
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

			// �ύ�����ν��вü�
			RasterizeTriangle_Clip(tri, g_context->viewport.n, g_context->viewport.f);
		}
	}
}

bool SrRasterizer::DrawLine( const float3& from, const float3& to )
{
	std::vector<float4>& vertices = g_context->IsFeatureEnable(eRFeature_TileBinning)
		? m_buildDynamicVertex : m_rendDynamicVertex;
	vertices.push_back( float4_from_v_w(from, 1.f) );
	vertices.push_back( float4_from_v_w(to, 1.f) );
	return true;
}

bool SrRasterizer::DrawRHZPrimitive( SrRendPrimitve& rendPrimitive )
{
	std::list<SrRendPrimitve*>& draws = g_context->IsFeatureEnable(eRFeature_TileBinning)
		? m_buildRendPrimitives : m_rendPrimitives;
	std::list<SrRendPrimitve*>& rhzDraws = g_context->IsFeatureEnable(eRFeature_TileBinning)
		? m_buildRendPrimitivesRHZ : m_rendPrimitivesRHZ;
	rendPrimitive.drawSequence = static_cast<uint32>(draws.size() + rhzDraws.size());
	rendPrimitive.frameNumber = m_renderer->getFrameCount();
	rendPrimitive.zNear = g_context->viewport.n;
	rendPrimitive.zFar = g_context->viewport.f;
	// ���shaderConstants
// 	rendPrimitive.shaderConstants.matrixs = m_renderer->m_matrixStack; // matrixStack����
// 	rendPrimitive.shaderConstants.textureStage = m_renderer->m_textureStages; // ����stage����
// 	rendPrimitive.shaderConstants.lightList = gEnv->sceneMgr->m_lightList; // lightList����
// 	rendPrimitive.shaderConstants.cBuffer = rendPrimitive.material->m_cbuffer; // ���ʵ�cBuffer����
// 	rendPrimitive.shaderConstants.alphaTest = true;
// 	rendPrimitive.shaderConstants.culling = false;

	rhzDraws.push_back(&rendPrimitive);
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
