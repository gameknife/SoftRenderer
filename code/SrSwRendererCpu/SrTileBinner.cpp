#include "SrTileRenderer.h"

void SrBuildTileBins(const SrTileLayout& layout,
	const std::vector<SrPreparedTriangleBounds>& bounds,
	SrTileBins& bins)
{
	const uint32_t tileCount = layout.GetTileCount();
	bins.offsets.assign(tileCount + 1, 0);
	for (uint32_t triangle = 0; triangle < bounds.size(); ++triangle)
	{
		const SrPreparedTriangleBounds& b = bounds[triangle];
		for (uint32_t ty = b.minY / layout.GetTileSize(); ty <= b.maxY / layout.GetTileSize(); ++ty)
			for (uint32_t tx = b.minX / layout.GetTileSize(); tx <= b.maxX / layout.GetTileSize(); ++tx)
				++bins.offsets[ty * layout.GetColumns() + tx + 1];
	}
	for (uint32_t i = 1; i < bins.offsets.size(); ++i)
		bins.offsets[i] += bins.offsets[i - 1];
	bins.triangleRefs.resize(bins.offsets.back());
	std::vector<uint32_t> cursors(bins.offsets.begin(), bins.offsets.end() - 1);
	for (uint32_t triangle = 0; triangle < bounds.size(); ++triangle)
	{
		const SrPreparedTriangleBounds& b = bounds[triangle];
		for (uint32_t ty = b.minY / layout.GetTileSize(); ty <= b.maxY / layout.GetTileSize(); ++ty)
			for (uint32_t tx = b.minX / layout.GetTileSize(); tx <= b.maxX / layout.GetTileSize(); ++tx)
			{
				const uint32_t tile = ty * layout.GetColumns() + tx;
				bins.triangleRefs[cursors[tile]++] = triangle;
			}
	}
}
