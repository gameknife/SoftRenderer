/**
  @file SrProfiler.cpp
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */
#include "StdAfx.h"
#include "SrProfiler.h"

#include "mmgr/mmgr.h"

SrProfiler::SrProfiler()
	: m_tileBusyRatioSum(0.f), m_tileBalanceSum(0.f), m_tileBatchCount(0)
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

void SrProfiler::addWorkerStats(uint32 workerIndex, uint32 jobs, float busyMilliseconds)
{
	if (m_workerStats.size() <= workerIndex)
		m_workerStats.resize(workerIndex + 1);
	m_workerStats[workerIndex].jobs += jobs;
	m_workerStats[workerIndex].busyMilliseconds += busyMilliseconds;
}

void SrProfiler::addTileBatchStats(float busyRatio, float balance)
{
	m_tileBusyRatioSum += busyRatio;
	m_tileBalanceSum += balance;
	++m_tileBatchCount;
}

void SrProfiler::Update()
{
	char buffer[512];
	sprintf_s(buffer, "SoftRENDERER v0.3(%s)| Fps: %.2f | FT: %.2fms | %d x %d |%s | %s | %s", 
		gEnv->renderer->getName(),
		1000.f / gEnv->profiler->getAverageTime(ePe_FrameTime), 
		gEnv->profiler->getAverageTime(ePe_FrameTime),
		g_context->width, g_context->height,
		g_context->IsFeatureEnable(eRFeature_MThreadRendering) ? "MT" : "ST",
		g_context->IsFeatureEnable(eRFeature_JitAA) ? "jitAA on" : "jitAA off",
		g_context->IsFeatureEnable(eRFeature_LinearFiltering) ? "LinearF" : "PointF"
		);
	gEnv->renderer->DrawScreenText( buffer, 20, 5, 0, SR_UICOLOR_MAIN );

	sprintf_s(buffer, "DP:%5.2fms | Flush:%5.2fms | TXT:%5.2fms | CLT:%5.2fms | VST:%5.2fms | RST:%5.2fms | PST:%5.2fms | PPT:%5.2fms ",

		gEnv->profiler->getAverageTime(ePe_DrawCallTime),
		//gEnv->profiler->getAverageTime(ePe_DrawCallAllocTime),
		gEnv->profiler->getAverageTime(ePe_FlushTime),
		gEnv->profiler->getAverageTime(ePe_FlushTextTime),
		gEnv->profiler->getAverageTime(ePe_ClearTime),
		gEnv->profiler->getAverageTime(ePe_VertexShaderTime),
		gEnv->profiler->getAverageTime(ePe_RasterizeShaderTime),
		gEnv->profiler->getAverageTime(ePe_PixelShaderTime),
		gEnv->profiler->getAverageTime(ePe_PostProcessTime));
		gEnv->renderer->DrawScreenText( buffer, 20, 20, 1, SR_UICOLOR_NORMAL );

	sprintf_s(buffer, "Batch: %3d | Tri(in): %6d | Pixel: %6d | Vertex: %6d",
		gEnv->profiler->getCount(ePe_BatchCount),
		gEnv->profiler->getCount(ePe_TriangleCount),
		gEnv->profiler->getCount(ePe_PixelCount),
		gEnv->profiler->getCount(ePe_VertexCount)
		);
	gEnv->renderer->DrawScreenText( buffer, 20, 30, 1, SR_UICOLOR_NORMAL );

	const std::vector<SrWorkerFrameStats>& workers = getWorkerStats();
	uint32 workerJobs = 0;
	uint32 activeWorkers = 0;
	float totalBusy = 0.f;
	float maxBusy = 0.f;
	for (uint32 i = 0; i < workers.size(); ++i)
	{
		workerJobs += workers[i].jobs;
		totalBusy += workers[i].busyMilliseconds;
		maxBusy = (std::max)(maxBusy, workers[i].busyMilliseconds);
		if (workers[i].jobs > 0) ++activeWorkers;
	}
	const float meanBusy = activeWorkers ? totalBusy / activeWorkers : 0.f;
	sprintf_s(buffer, "Workers: %2u/%2u | Jobs: %6u | Busy mean/max: %5.2f/%5.2fms | balance: %.2f",
		activeWorkers, static_cast<uint32>(workers.size()), workerJobs, meanBusy, maxBusy,
		meanBusy > 0.f ? maxBusy / meanBusy : 0.f);
	gEnv->renderer->DrawScreenText( buffer, 20, 40, 1, SR_UICOLOR_NORMAL );

	if (gEnv->profiler->getCount(ePe_TileCount) > 0)
	{
		sprintf_s(buffer, "Tile: %d | count/jobs: %d/%d | active/empty: %d/%d | refs: %d | SSAO: %s | busy/bal: %.0f%%/%.2f",
			gEnv->profiler->getCount(ePe_TileSize),
			gEnv->profiler->getCount(ePe_TileCount),
			gEnv->profiler->getCount(ePe_TileJobCount),
			gEnv->profiler->getCount(ePe_ActiveTileCount),
			gEnv->profiler->getCount(ePe_EmptyTileCount),
			gEnv->profiler->getCount(ePe_BinReferenceCount),
			gEnv->profiler->getCount(ePe_RelaxedSsao) ? "relaxed" : "exact",
			getTileBusyRatio() * 100.f, getTileWorkerBalance());
		gEnv->renderer->DrawScreenText(buffer, 20, 50, 1, SR_UICOLOR_NORMAL);
	}

	// profile resources


	for (int i = 0; i < ePe_Count; i++)
	{
		m_profileElements[i].Clear();
	}
	m_workerStats.clear();
	m_tileBusyRatioSum = 0.f;
	m_tileBalanceSum = 0.f;
	m_tileBatchCount = 0;
}
