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

void SrProfiler::Update()
{
	sprintf(m_buffer, "SoftRENDERER v0.3(%s)| Fps: %.2f | FT: %.2fms | %d x %d |%s | %s | %s <br> "
		"DP:%5.2fms | Flush:%5.2fms | CLT:%5.2fms | VST:%5.2fms | RST:%5.2fms | PST:%5.2fms | PPT:%5.2fms <br> "
		"Batch: %3d | Tri: %6d | Pixel: %6d | Vertex: %6d <br> "
		"Processor: %d | Task M/0/1/2/3/4/5/6: %4d/%4d/%4d/%4d/%4d/%4d/%4d/%4d <br> ",
		gEnv->renderer->getName(),
		1000.f / gEnv->profiler->getAverageTime(ePe_FrameTime),
		gEnv->profiler->getAverageTime(ePe_FrameTime),
		g_context->width, g_context->height,
		g_context->IsFeatureEnable(eRFeature_MThreadRendering) ? "MT" : "ST",
		g_context->IsFeatureEnable(eRFeature_JitAA) ? "jitAA on" : "jitAA off",
		g_context->IsFeatureEnable(eRFeature_LinearFiltering) ? "LinearF" : "PointF",

		gEnv->profiler->getAverageTime(ePe_DrawCallTime),
		gEnv->profiler->getAverageTime(ePe_FlushTime),
		gEnv->profiler->getAverageTime(ePe_ClearTime),
		gEnv->profiler->getAverageTime(ePe_VertexShaderTime),
		gEnv->profiler->getAverageTime(ePe_RasterizeShaderTime),
		gEnv->profiler->getAverageTime(ePe_PixelShaderTime),
		gEnv->profiler->getAverageTime(ePe_PostProcessTime),

		gEnv->profiler->getCount(ePe_BatchCount),
		gEnv->profiler->getCount(ePe_TriangleCount),
		gEnv->profiler->getCount(ePe_PixelCount),
		gEnv->profiler->getCount(ePe_VertexCount),

		g_context->processorNum,
		gEnv->profiler->getCount(ePe_MainThreadTaskNum),
		gEnv->profiler->getCount(ePe_Thread0TaskNum),
		gEnv->profiler->getCount(ePe_Thread1TaskNum),
		gEnv->profiler->getCount(ePe_Thread2TaskNum),
		gEnv->profiler->getCount(ePe_Thread3TaskNum),
		gEnv->profiler->getCount(ePe_Thread4TaskNum),
		gEnv->profiler->getCount(ePe_Thread5TaskNum),
		gEnv->profiler->getCount(ePe_Thread6TaskNum)
	);

	// profile resources
	for (int i = 0; i < ePe_Count; i++)
	{
		m_profileElements[i].Clear();
	}
}
