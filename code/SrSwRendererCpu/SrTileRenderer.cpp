#include "StdAfx.h"
#include "SrTileRenderer.h"
#include "SrRasterizer.h"
#include "SrRasTaskDispatcher.h"
#include "SrRasTasks.h"
#include "SrFragmentBuffer.h"
#include "SrSoftRenderer.h"
#include "SrSwShader.h"
#include "SrProfiler.h"
#include <atomic>
#include <cstdlib>

namespace
{
	struct SrTileFrameJobs
	{
		SrTileLayout* layout;
		SrTileBins* bins;
		std::vector<SrRasterizer::SrRastTriangle*>* triangles;
		uint32* outBuffer;
		uint32 outPitchPixels;
		uint32* memBuffer;
		uint32* backBuffer;
		uint32* gpuBuffer;
		uint32 gpuPitchPixels;
		bool temporalPost;
		bool fusePost;
		bool relaxedDeferred;
		std::atomic<LONG>* tileReady;
		std::vector<std::vector<uint32> >* deferredPixels;
		std::vector<uint32>* shadedPixels;
	};

	struct SrGeometryJobs
	{
		std::vector<SrRendPrimitve*>* draws;
		std::vector<std::vector<SrRasterizer::SrRastTriangle> >* outputs;
		std::atomic<uint32> vertexCount;
	};

	struct SrBinnerJobs
	{
		const SrTileLayout* layout;
		std::vector<SrPreparedTriangleBounds>* bounds;
		uint32 chunkCount;
		std::vector<uint32>* counts;
		std::vector<uint32>* starts;
		SrTileBins* bins;
		std::vector<SrRasterizer::SrRastTriangle*>* triangles;
		std::vector<uint8>* dependencyFlags;
	};

	static void ShadeLocalAndCollect(uint32 tileIndex, const SrTileRect& rect, SrTileFrameJobs& jobs)
	{
		uint32 shadedPixels = 0;
		std::vector<uint32>& deferredPixels = (*jobs.deferredPixels)[tileIndex];
		for (uint32 y = rect.y0; y < rect.y1; ++y)
			for (uint32 x = rect.x0; x < rect.x1; ++x)
			{
				const uint32 address = y * g_context->width + x;
				if (fBuffer->zBuffer[address] >= 0.f)
					continue;
				SrFragment* fragment = fBuffer->fBuffer + address;
				if (!fragment->primitive || !fragment->primitive->shader)
					continue;
				const bool needsDeferred = (fragment->primitive->shader->GetPixelDependencyFlags() & ePd_ReadsFullGBuffer) != 0;
				if (needsDeferred)
				{
					deferredPixels.push_back(address);
					continue;
				}
				uint32* output = jobs.outBuffer + y * jobs.outPitchPixels + x;
				fragment->primitive->shader->ProcessPixel(output, fragment,
					&fragment->primitive->shaderConstants, address);
				++shadedPixels;
			}
		(*jobs.shadedPixels)[tileIndex] += shadedPixels;
	}

	static void PostTile(const SrTileRect& rect, SrTileFrameJobs& jobs)
	{
		for (uint32 y = rect.y0; y < rect.y1; ++y)
			for (uint32 x = rect.x0; x < rect.x1; ++x)
			{
				const uint32 address = y * g_context->width + x;
				const bool coverage = g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) != FALSE;
				jobs.gpuBuffer[y * jobs.gpuPitchPixels + x] = SrColorMerge(jobs.memBuffer[address], jobs.backBuffer[address], !coverage);
				jobs.backBuffer[address] = jobs.memBuffer[address];
			}
	}

	static void ShadeDeferredTile(uint32 tileIndex, const SrTileRect& rect, SrTileFrameJobs& jobs)
	{
		const std::vector<uint32>& pixels = (*jobs.deferredPixels)[tileIndex];
		for (uint32 i = 0; i < pixels.size(); ++i)
		{
			const uint32 address = pixels[i];
			const uint32 y = address / g_context->width;
			const uint32 x = address - y * g_context->width;
			SrFragment* fragment = fBuffer->fBuffer + address;
			uint32* output = jobs.outBuffer + y * jobs.outPitchPixels + x;
			fragment->primitive->shader->ProcessPixel(output, fragment,
				&fragment->primitive->shaderConstants, address);
		}
		(*jobs.shadedPixels)[tileIndex] += static_cast<uint32>(pixels.size());
		if (jobs.temporalPost)
			PostTile(rect, jobs);
	}

	static void RasterTileJob(uint32 tileIndex, void* opaque)
	{
		SrTileFrameJobs& jobs = *static_cast<SrTileFrameJobs*>(opaque);
		const SrTileRect rect = jobs.layout->GetRect(tileIndex);
		for (uint32 y = rect.y0; y < rect.y1; ++y)
		{
			memset(jobs.outBuffer + y * jobs.outPitchPixels + rect.x0, SR_GREYSCALE_CLEARCOLOR,
				(rect.x1 - rect.x0) * sizeof(uint32));
			const uint32 address = y * g_context->width + rect.x0;
			memset(fBuffer->fBuffer + address, 0, (rect.x1 - rect.x0) * sizeof(SrFragment));
			memset(fBuffer->zBuffer + address, 0, (rect.x1 - rect.x0) * sizeof(float));
		}

		SrRasterizer::SetTileRect(&rect);
		for (uint32 ref = jobs.bins->offsets[tileIndex]; ref < jobs.bins->offsets[tileIndex + 1]; ++ref)
		{
			SrRasterizer::SrRastTriangle triangle = *(*jobs.triangles)[jobs.bins->triangleRefs[ref]];
			SrRasterizer::RasterizeTriangle(triangle);
		}
		SrRasterizer::SetTileRect(NULL);
		ShadeLocalAndCollect(tileIndex, rect, jobs);
		if (jobs.relaxedDeferred)
		{
			// Publish all GBuffer writes for this tile before a different tile is
			// allowed to sample them.
			jobs.tileReady[tileIndex].store(1, std::memory_order_release);
			ShadeDeferredTile(tileIndex, rect, jobs);
		}
		else if (jobs.fusePost)
			PostTile(rect, jobs);
	}

	static void DeferredTileJob(uint32 tileIndex, void* opaque)
	{
		SrTileFrameJobs& jobs = *static_cast<SrTileFrameJobs*>(opaque);
		const SrTileRect rect = jobs.layout->GetRect(tileIndex);
		ShadeDeferredTile(tileIndex, rect, jobs);
	}

	static void PostTileJob(uint32 tileIndex, void* opaque)
	{
		SrTileFrameJobs& jobs = *static_cast<SrTileFrameJobs*>(opaque);
		PostTile(jobs.layout->GetRect(tileIndex), jobs);
	}

	static void CollectPrimitive(SrRendPrimitve* primitive)
	{
		if (!primitive || !primitive->vb || !primitive->ib)
			return;
		const uint32 triangleCount = primitive->ib->count / 3;
		for (uint32 i = 0; i < triangleCount; ++i)
		{
			SrRasterizer::SrRastTriangle triangle;
			triangle.p[0] = *reinterpret_cast<SrRendVertex*>(primitive->vb->data + primitive->ib->data[i * 3] * primitive->vb->elementSize);
			triangle.p[1] = *reinterpret_cast<SrRendVertex*>(primitive->vb->data + primitive->ib->data[i * 3 + 1] * primitive->vb->elementSize);
			triangle.p[2] = *reinterpret_cast<SrRendVertex*>(primitive->vb->data + primitive->ib->data[i * 3 + 2] * primitive->vb->elementSize);
			triangle.primitive = primitive;
			SrRasterizer::RasterizeTriangle_Clip(triangle, g_context->viewport.n, g_context->viewport.f);
		}
	}

	static void GeometryJob(uint32 drawIndex, void* opaque)
	{
		SrGeometryJobs& jobs = *static_cast<SrGeometryJobs*>(opaque);
		SrRendPrimitve* primitive = (*jobs.draws)[drawIndex];
		for (uint32 i = 0; i < primitive->vb->elementCount; ++i)
		{
			void* output = primitive->vb->data + i * primitive->vb->elementSize;
			const SrRendVertex input = *static_cast<SrRendVertex*>(output);
			primitive->shader->ProcessVertex(output, NULL, NULL, &input, NULL, NULL, &primitive->shaderConstants);
		}
		jobs.vertexCount.fetch_add(primitive->vb->elementCount, std::memory_order_relaxed);
		SrRasterizer::SetTriangleCollector(&(*jobs.outputs)[drawIndex]);
		CollectPrimitive(primitive);
		SrRasterizer::SetTriangleCollector(NULL);
	}

	static void BinCountJob(uint32 chunk, void* opaque)
	{
		SrBinnerJobs& jobs = *static_cast<SrBinnerJobs*>(opaque);
		const uint32 tileCount = jobs.layout->GetTileCount();
		uint32* counts = &(*jobs.counts)[chunk * tileCount];
		const uint32 begin = static_cast<uint32>((static_cast<uint64_t>(jobs.bounds->size()) * chunk) / jobs.chunkCount);
		const uint32 end = static_cast<uint32>((static_cast<uint64_t>(jobs.bounds->size()) * (chunk + 1)) / jobs.chunkCount);
		bool readsFullGBuffer = false;
		for (uint32 triangle = begin; triangle < end; ++triangle)
		{
			SrRasterizer::SrRastTriangle& prepared = *(*jobs.triangles)[triangle];
			float minX = prepared.p[0].pos.x, maxX = minX;
			float minY = prepared.p[0].pos.y, maxY = minY;
			for (uint32 vertex = 1; vertex < 3; ++vertex)
			{
				minX = (std::min)(minX, prepared.p[vertex].pos.x); maxX = (std::max)(maxX, prepared.p[vertex].pos.x);
				minY = (std::min)(minY, prepared.p[vertex].pos.y); maxY = (std::max)(maxY, prepared.p[vertex].pos.y);
			}
			SrPreparedTriangleBounds& b = (*jobs.bounds)[triangle];
			b.minX = static_cast<uint32>(Clamp(floor(minX), 0.f, static_cast<float>(g_context->width - 1)));
			b.minY = static_cast<uint32>(Clamp(floor(minY), 0.f, static_cast<float>(g_context->height - 1)));
			b.maxX = static_cast<uint32>(Clamp(ceil(maxX), 0.f, static_cast<float>(g_context->width - 1)));
			b.maxY = static_cast<uint32>(Clamp(ceil(maxY), 0.f, static_cast<float>(g_context->height - 1)));
			readsFullGBuffer |= (prepared.primitive->shader->GetPixelDependencyFlags() & ePd_ReadsFullGBuffer) != 0;
			for (uint32 ty = b.minY / jobs.layout->GetTileSize(); ty <= b.maxY / jobs.layout->GetTileSize(); ++ty)
				for (uint32 tx = b.minX / jobs.layout->GetTileSize(); tx <= b.maxX / jobs.layout->GetTileSize(); ++tx)
					++counts[ty * jobs.layout->GetColumns() + tx];
		}
		(*jobs.dependencyFlags)[chunk] = readsFullGBuffer ? 1 : 0;
	}

	static void BinScatterJob(uint32 chunk, void* opaque)
	{
		SrBinnerJobs& jobs = *static_cast<SrBinnerJobs*>(opaque);
		const uint32 tileCount = jobs.layout->GetTileCount();
		uint32* cursors = &(*jobs.starts)[chunk * tileCount];
		const uint32 begin = static_cast<uint32>((static_cast<uint64_t>(jobs.bounds->size()) * chunk) / jobs.chunkCount);
		const uint32 end = static_cast<uint32>((static_cast<uint64_t>(jobs.bounds->size()) * (chunk + 1)) / jobs.chunkCount);
		for (uint32 triangle = begin; triangle < end; ++triangle)
		{
			const SrPreparedTriangleBounds& b = (*jobs.bounds)[triangle];
			for (uint32 ty = b.minY / jobs.layout->GetTileSize(); ty <= b.maxY / jobs.layout->GetTileSize(); ++ty)
				for (uint32 tx = b.minX / jobs.layout->GetTileSize(); tx <= b.maxX / jobs.layout->GetTileSize(); ++tx)
				{
					const uint32 tile = ty * jobs.layout->GetColumns() + tx;
					jobs.bins->triangleRefs[cursors[tile]++] = triangle;
				}
		}
	}
}

void SrRasterizer::FlushTileBinning(uint32* outBuffer, uint32* memBuffer, uint32* backBuffer, uint32* gpuBuffer)
{
	gEnv->profiler->setBegin(ePe_VertexShaderTime);
	m_tileDraws.assign(m_rendPrimitives.begin(), m_rendPrimitives.end());
	int submittedTriangleCount = 0;
	for (uint32 draw = 0; draw < m_tileDraws.size(); ++draw)
		if (m_tileDraws[draw] && m_tileDraws[draw]->vb && m_tileDraws[draw]->ib)
			submittedTriangleCount += static_cast<int>(m_tileDraws[draw]->ib->count / 3);
	for (std::list<SrRendPrimitve*>::iterator it = m_rendPrimitivesRHZ.begin(); it != m_rendPrimitivesRHZ.end(); ++it)
		if (*it && (*it)->vb && (*it)->ib)
			submittedTriangleCount += static_cast<int>((*it)->ib->count / 3);
	gEnv->profiler->setIncrement(ePe_TriangleCount, submittedTriangleCount);
	if (m_tileDrawTriangles.size() < m_tileDraws.size())
		m_tileDrawTriangles.resize(m_tileDraws.size());
	for (uint32 draw = 0; draw < m_tileDraws.size(); ++draw)
		m_tileDrawTriangles[draw].clear();
	SrGeometryJobs geometryJobs;
	geometryJobs.draws = &m_tileDraws;
	geometryJobs.outputs = &m_tileDrawTriangles;
	geometryJobs.vertexCount.store(0);
	m_rasTaskDispatcher->RunBatch(static_cast<uint32>(m_tileDraws.size()), &geometryJobs, GeometryJob);
	gEnv->profiler->setIncrement(ePe_VertexCount, geometryJobs.vertexCount.load());
	m_tilePreparedTriangles.clear();
	uint32 preparedCount = 0;
	for (uint32 draw = 0; draw < m_tileDraws.size(); ++draw)
		preparedCount += static_cast<uint32>(m_tileDrawTriangles[draw].size());
	SetTriangleCollector(&m_tilePreparedTriangles);
	for (std::list<SrRendPrimitve*>::iterator it = m_rendPrimitivesRHZ.begin(); it != m_rendPrimitivesRHZ.end(); ++it)
		CollectPrimitive(*it);
	SetTriangleCollector(NULL);
	preparedCount += static_cast<uint32>(m_tilePreparedTriangles.size());
	m_tilePreparedTriangleRefs.clear();
	m_tilePreparedTriangleRefs.reserve(preparedCount);
	for (uint32 draw = 0; draw < m_tileDraws.size(); ++draw)
		for (uint32 triangle = 0; triangle < m_tileDrawTriangles[draw].size(); ++triangle)
			m_tilePreparedTriangleRefs.push_back(&m_tileDrawTriangles[draw][triangle]);
	for (uint32 triangle = 0; triangle < m_tilePreparedTriangles.size(); ++triangle)
		m_tilePreparedTriangleRefs.push_back(&m_tilePreparedTriangles[triangle]);
	gEnv->profiler->setEnd(ePe_VertexShaderTime);

	// 24x24 is the measured default on the 854x480 reference workload: it
	// keeps enough jobs for the 24-way pool without the bin amplification of
	// 8/16 tiles. SR_TILE_SIZE remains the authoritative override.
	uint32 tileSize = 24;
	if (const char* value = std::getenv("SR_TILE_SIZE"))
		tileSize = (std::max<uint32>)(1, static_cast<uint32>(atoi(value)));
	m_tileLayout.Reset(g_context->width, g_context->height, tileSize);
	m_tileTriangleBounds.resize(m_tilePreparedTriangleRefs.size());
	gEnv->profiler->setBegin(ePe_BinningTime);
	bool hasFullGBufferShader = false;
	const uint32 participantCount = m_rasTaskDispatcher->GetParticipantCount();
	const uint32 chunkCount = m_tilePreparedTriangleRefs.empty() ? 0 : (std::min<uint32>)(static_cast<uint32>(m_tilePreparedTriangleRefs.size()), participantCount * 2);
	if (chunkCount > 0)
	{
		const uint32 tileCount = m_tileLayout.GetTileCount();
		m_tileBinCounts.assign(chunkCount * tileCount, 0);
		m_tileBinStarts.resize(chunkCount * tileCount);
		m_tileDependencyFlags.assign(chunkCount, 0);
		SrBinnerJobs binner = { &m_tileLayout, &m_tileTriangleBounds, chunkCount, &m_tileBinCounts, &m_tileBinStarts, &m_tileBins, &m_tilePreparedTriangleRefs, &m_tileDependencyFlags };
		gEnv->profiler->setBegin(ePe_BinCountTime);
		m_rasTaskDispatcher->RunBatch(chunkCount, &binner, BinCountJob);
		gEnv->profiler->setEnd(ePe_BinCountTime);
		for (uint32 chunk = 0; chunk < chunkCount; ++chunk)
			hasFullGBufferShader |= m_tileDependencyFlags[chunk] != 0;
		gEnv->profiler->setBegin(ePe_BinPrefixTime);
		m_tileBins.offsets.assign(tileCount + 1, 0);
		for (uint32 tile = 0; tile < tileCount; ++tile)
		{
			uint32 total = 0;
			for (uint32 chunk = 0; chunk < chunkCount; ++chunk)
			{
				m_tileBinStarts[chunk * tileCount + tile] = m_tileBins.offsets[tile] + total;
				total += m_tileBinCounts[chunk * tileCount + tile];
			}
			m_tileBins.offsets[tile + 1] = m_tileBins.offsets[tile] + total;
		}
		gEnv->profiler->setEnd(ePe_BinPrefixTime);
		m_tileBins.triangleRefs.resize(m_tileBins.offsets.back());
		gEnv->profiler->setBegin(ePe_BinScatterTime);
		m_rasTaskDispatcher->RunBatch(chunkCount, &binner, BinScatterJob);
		gEnv->profiler->setEnd(ePe_BinScatterTime);
	}
	else
	{
		m_tileBins.offsets.assign(m_tileLayout.GetTileCount() + 1, 0);
		m_tileBins.triangleRefs.clear();
	}
	gEnv->profiler->setEnd(ePe_BinningTime);
	uint32 activeTiles = 0;
	for (uint32 tile = 0; tile < m_tileLayout.GetTileCount(); ++tile)
		if (m_tileBins.offsets[tile] != m_tileBins.offsets[tile + 1]) ++activeTiles;
	gEnv->profiler->setIncrement(ePe_TileSize, tileSize);
	gEnv->profiler->setIncrement(ePe_TileCount, m_tileLayout.GetTileCount());
	gEnv->profiler->setIncrement(ePe_ActiveTileCount, activeTiles);
	gEnv->profiler->setIncrement(ePe_EmptyTileCount, m_tileLayout.GetTileCount() - activeTiles);
	gEnv->profiler->setIncrement(ePe_BinReferenceCount, static_cast<int>(m_tileBins.triangleRefs.size()));

	SrTileFrameJobs jobs;
	jobs.layout = &m_tileLayout; jobs.bins = &m_tileBins; jobs.triangles = &m_tilePreparedTriangleRefs;
	jobs.outBuffer = outBuffer;
	jobs.outPitchPixels = outBuffer == gpuBuffer && m_renderer->m_bufferPitch > 0 ? m_renderer->m_bufferPitch / sizeof(uint32) : g_context->width;
	jobs.memBuffer = memBuffer; jobs.backBuffer = backBuffer; jobs.gpuBuffer = gpuBuffer;
	jobs.gpuPitchPixels = m_renderer->m_bufferPitch > 0 ? m_renderer->m_bufferPitch / sizeof(uint32) : g_context->width;
	jobs.temporalPost = g_context->IsFeatureEnable(eRFeature_JitAA) || g_context->IsFeatureEnable(eRFeature_DotCoverageRendering);
	const char* relaxedSsaoValue = std::getenv("SR_RELAXED_SSAO");
	const bool relaxedSsao = hasFullGBufferShader && relaxedSsaoValue && atoi(relaxedSsaoValue) != 0;
	gEnv->profiler->setIncrement(ePe_RelaxedSsao, relaxedSsao ? 1 : 0);
	jobs.relaxedDeferred = relaxedSsao;
	jobs.fusePost = jobs.temporalPost && !hasFullGBufferShader;
	if (m_tileDeferredPixels.size() < m_tileLayout.GetTileCount())
		m_tileDeferredPixels.resize(m_tileLayout.GetTileCount());
	for (uint32 tile = 0; tile < m_tileLayout.GetTileCount(); ++tile)
		m_tileDeferredPixels[tile].clear();
	m_tileShadedPixels.assign(m_tileLayout.GetTileCount(), 0);
	jobs.deferredPixels = &m_tileDeferredPixels;
	jobs.shadedPixels = &m_tileShadedPixels;
	if (m_tileReady.size() != m_tileLayout.GetTileCount())
	{
		std::vector<std::atomic<LONG> > ready(m_tileLayout.GetTileCount());
		m_tileReady.swap(ready);
	}
	if (relaxedSsao)
		for (uint32 tile = 0; tile < m_tileLayout.GetTileCount(); ++tile)
			m_tileReady[tile].store(0, std::memory_order_relaxed);
	jobs.tileReady = relaxedSsao ? &m_tileReady[0] : NULL;
	if (relaxedSsao)
		fBuffer->ConfigureRelaxedTileReads(jobs.tileReady, m_tileLayout.GetTileSize(), m_tileLayout.GetColumns());

	gEnv->profiler->setEnd(ePe_ClearTime);
	gEnv->profiler->setBegin(ePe_RasterizeShaderTime);
	m_rasTaskDispatcher->RunBatch(m_tileLayout.GetTileCount(), &jobs, RasterTileJob);
	if (relaxedSsao)
		fBuffer->DisableRelaxedTileReads();
	gEnv->profiler->addTileBatchStats(m_rasTaskDispatcher->GetLastBatchBusyRatio(), m_rasTaskDispatcher->GetLastBatchBalance());
	gEnv->profiler->setIncrement(ePe_TileJobCount, m_tileLayout.GetTileCount());
	gEnv->profiler->setEnd(ePe_RasterizeShaderTime);

	bool hasDeferredWinner = false;
	for (uint32 tile = 0; tile < m_tileLayout.GetTileCount(); ++tile)
		hasDeferredWinner |= !m_tileDeferredPixels[tile].empty();
	if (!relaxedSsao && hasFullGBufferShader && hasDeferredWinner)
	{
		gEnv->profiler->setBegin(ePe_PixelShaderTime);
		m_rasTaskDispatcher->RunBatch(m_tileLayout.GetTileCount(), &jobs, DeferredTileJob);
		gEnv->profiler->addTileBatchStats(m_rasTaskDispatcher->GetLastBatchBusyRatio(), m_rasTaskDispatcher->GetLastBatchBalance());
		gEnv->profiler->setIncrement(ePe_TileJobCount, m_tileLayout.GetTileCount());
		gEnv->profiler->setEnd(ePe_PixelShaderTime);
	}
	else if (!relaxedSsao && jobs.temporalPost && !jobs.fusePost)
	{
		m_rasTaskDispatcher->RunBatch(m_tileLayout.GetTileCount(), &jobs, PostTileJob);
		gEnv->profiler->addTileBatchStats(m_rasTaskDispatcher->GetLastBatchBusyRatio(), m_rasTaskDispatcher->GetLastBatchBalance());
		gEnv->profiler->setIncrement(ePe_TileJobCount, m_tileLayout.GetTileCount());
	}
	uint32 shadedPixels = 0;
	for (uint32 tile = 0; tile < m_tileLayout.GetTileCount(); ++tile)
		shadedPixels += m_tileShadedPixels[tile];
	gEnv->profiler->setIncrement(ePe_PixelCount, shadedPixels);
}
