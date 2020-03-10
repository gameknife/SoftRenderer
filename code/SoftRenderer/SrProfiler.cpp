/**
  @file SrProfiler.cpp
  
  @author Kaiming

  ������־ history
  ver:1.0
   
 */
#include "stdafx.h"
#include "SrProfiler.h"

SrProfiler::SrProfiler()
{
	m_profileElements.assign(ePe_Count, SrProfilerElement());
	m_profileElements[ePe_ZError].dontClear = true;
}

SrProfiler::~SrProfiler()
{
}


void SrProfiler::setBegin( EProfilerElement element )
{
	m_profileElements[element].prevTime = gEnv->timer->getRealTime();
}

void SrProfiler::setEnd( EProfilerElement element )
{
	SrProfilerElement& elementInst = m_profileElements[element];
	float now = gEnv->timer->getRealTime();
	elementInst.elapsedTime = now - elementInst.prevTime;
}


void SrProfiler::IncreaseTime( EProfilerElement element, float time )
{
	m_profileElements[element].elapsedTime += time;
}


void SrProfiler::setIncrement( EProfilerElement element, int count /*= 1 */ )
{
	m_profileElements[element].count += count;
}

float SrProfiler::getTime( EProfilerElement element )
{
	return m_profileElements[element].elapsedTime * 1000.f;
}

float SrProfiler::getAverageTime( EProfilerElement element )
{
	return m_profileElements[element].avgElapsedTime * 1000.f;
}

int SrProfiler::getCount( EProfilerElement element )
{
	return m_profileElements[element].count;
}

char* SrProfiler::getProfileData()
{
	return m_buffer;
}

char* SrProfiler::getProfileDataConsole()
{
	return m_buffer_simple;
}

void SrProfiler::Update()
{
	sprintf(m_buffer_simple, "  Fps: %.2f | FT: %.2fms | %d x %d |%s | %s | %s | "
		"Wait:%5.2fms | Flush:%5.2fms | CLT:%5.2fms | VST:%5.2fms | RST:%5.2fms | PST:%5.2fms | PPT:%5.2fms | "
		"Processor: %d",
		1000.f / gEnv->profiler->getTime(ePe_FrameTime),
		gEnv->profiler->getTime(ePe_FrameTime),
		g_context->width, g_context->height,
		g_context->IsFeatureEnable(eRFeature_MThreadRendering) ? "MT" : "ST",
		g_context->IsFeatureEnable(eRFeature_JitAA) ? "jitAA on" : "jitAA off",
		g_context->IsFeatureEnable(eRFeature_LinearFiltering) ? "LinearF" : "PointF",

		gEnv->profiler->getTime(ePe_DispatcherWaitTime),
		gEnv->profiler->getTime(ePe_FlushTime),
		gEnv->profiler->getTime(ePe_ClearTime),
		gEnv->profiler->getTime(ePe_VertexShaderTime),
		gEnv->profiler->getTime(ePe_RasterizeShaderTime),
		gEnv->profiler->getTime(ePe_PixelShaderTime),
		gEnv->profiler->getTime(ePe_PostProcessTime),
		g_context->processorNum
	);
	
	sprintf(m_buffer, "SoftRENDERER v0.3(%s)| Fps: %.2f | FT: %.2fms | %d x %d |%s | %s | %s <br> "
		"DP:%5.2fms | DispWait: %5.2fms | Flush:%5.2fms | CLT:%5.2fms | VST:%5.2fms | RST:%5.2fms | RSFT:%5.2fms | PST:%5.2fms | PSFT:%5.2fms | PPT:%5.2fms | PCT:%5.2fms <br> "
		"Batch: %3d | Tri: %6d | Pixel: %6d | Vertex: %6d <br> "
		"Processor: %d | Task PerThread: <br> %4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d <br> %4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d <br>"
		"Processor: %d | Task Idle: <br> %5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f <br> %5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f/%5.2f <br>",
		gEnv->renderer->getName(),
		1000.f / gEnv->profiler->getAverageTime(ePe_FrameTime),
		gEnv->profiler->getAverageTime(ePe_FrameTime),
		g_context->width, g_context->height,
		g_context->IsFeatureEnable(eRFeature_MThreadRendering) ? "MT" : "ST",
		g_context->IsFeatureEnable(eRFeature_JitAA) ? "jitAA on" : "jitAA off",
		g_context->IsFeatureEnable(eRFeature_LinearFiltering) ? "LinearF" : "PointF",

		gEnv->profiler->getAverageTime(ePe_DrawCallTime),
		gEnv->profiler->getAverageTime(ePe_DispatcherWaitTime),
		gEnv->profiler->getAverageTime(ePe_FlushTime),
		gEnv->profiler->getAverageTime(ePe_ClearTime),
		gEnv->profiler->getAverageTime(ePe_VertexShaderTime),
		gEnv->profiler->getAverageTime(ePe_RasterizeShaderTime),
		gEnv->profiler->getAverageTime(ePe_RasterizeShaderFlushTime),
		gEnv->profiler->getAverageTime(ePe_PixelShaderTime),
		gEnv->profiler->getAverageTime(ePe_PixelShaderFlushTime),
		gEnv->profiler->getAverageTime(ePe_PostProcessTime),
		gEnv->profiler->getAverageTime(ePe_PostClearTime),
		

		gEnv->profiler->getCount(ePe_BatchCount),
		gEnv->profiler->getCount(ePe_TriangleCount),
		gEnv->profiler->getCount(ePe_PixelCount),
		gEnv->profiler->getCount(ePe_VertexCount),

		g_context->processorNum,
		//gEnv->profiler->getCount(ePe_MainThreadTaskNum),
		gEnv->profiler->getCount(ePe_Thread0TaskNum),
		gEnv->profiler->getCount(ePe_Thread1TaskNum),
		gEnv->profiler->getCount(ePe_Thread2TaskNum),
		gEnv->profiler->getCount(ePe_Thread3TaskNum),
		gEnv->profiler->getCount(ePe_Thread4TaskNum),
		gEnv->profiler->getCount(ePe_Thread5TaskNum),
		gEnv->profiler->getCount(ePe_Thread6TaskNum),
		gEnv->profiler->getCount(ePe_Thread7TaskNum),
		gEnv->profiler->getCount(ePe_Thread8TaskNum),
		gEnv->profiler->getCount(ePe_Thread9TaskNum),
		gEnv->profiler->getCount(ePe_Thread10TaskNum),
		gEnv->profiler->getCount(ePe_Thread11TaskNum),
		gEnv->profiler->getCount(ePe_Thread12TaskNum),
		gEnv->profiler->getCount(ePe_Thread13TaskNum),
		gEnv->profiler->getCount(ePe_Thread14TaskNum),
		gEnv->profiler->getCount(ePe_Thread15TaskNum),
		gEnv->profiler->getCount(ePe_Thread16TaskNum),
		gEnv->profiler->getCount(ePe_Thread17TaskNum),
		gEnv->profiler->getCount(ePe_Thread18TaskNum),
		gEnv->profiler->getCount(ePe_Thread19TaskNum),
		gEnv->profiler->getCount(ePe_Thread20TaskNum),
		gEnv->profiler->getCount(ePe_Thread21TaskNum),
		gEnv->profiler->getCount(ePe_Thread22TaskNum),
		gEnv->profiler->getCount(ePe_Thread23TaskNum),

		g_context->processorNum,
		gEnv->profiler->getAverageTime(ePe_Thread0IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread1IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread2IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread3IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread4IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread5IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread6IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread7IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread8IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread9IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread10IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread11IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread12IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread13IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread14IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread15IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread16IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread17IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread18IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread19IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread20IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread21IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread22IdleTime),
		gEnv->profiler->getAverageTime(ePe_Thread23IdleTime)
	);

	// profile resources
	for (int i = 0; i < ePe_Count; i++)
	{
		m_profileElements[i].Clear();
	}
}
