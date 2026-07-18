#include "SrTileTypes.h"
#include "SrTileRenderer.h"
#include <cassert>
#include <vector>

static void CheckLayout(uint32_t width, uint32_t height, uint32_t tileSize)
{
	SrTileLayout layout(width, height, tileSize);
	std::vector<unsigned char> owners(width * height, 0);
	for (uint32_t tile = 0; tile < layout.GetTileCount(); ++tile)
	{
		const SrTileRect rect = layout.GetRect(tile);
		assert(rect.x0 < rect.x1 && rect.y0 < rect.y1);
		assert(rect.x1 <= width && rect.y1 <= height);
		for (uint32_t y = rect.y0; y < rect.y1; ++y)
			for (uint32_t x = rect.x0; x < rect.x1; ++x)
			{
				assert(layout.GetTileIndex(x, y) == tile);
				++owners[y * width + x];
			}
	}
	for (size_t i = 0; i < owners.size(); ++i)
		assert(owners[i] == 1);
}

int main()
{
	const uint32_t resolutions[][2] = {{1,1}, {31,17}, {32,32}, {33,33}, {854,480}, {1280,720}, {1920,1080}};
	const uint32_t sizes[] = {8,16,32,64};
	for (size_t r = 0; r < sizeof(resolutions) / sizeof(resolutions[0]); ++r)
		for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s)
			CheckLayout(resolutions[r][0], resolutions[r][1], sizes[s]);

	SrTileLayout layout(64, 64, 32);
	std::vector<SrPreparedTriangleBounds> bounds;
	bounds.push_back(SrPreparedTriangleBounds{1, 1, 10, 10});
	bounds.push_back(SrPreparedTriangleBounds{16, 16, 48, 48});
	bounds.push_back(SrPreparedTriangleBounds{40, 1, 63, 20});
	SrTileBins bins;
	SrBuildTileBins(layout, bounds, bins);
	assert((bins.offsets == std::vector<uint32_t>{0, 2, 4, 5, 6}));
	assert((bins.triangleRefs == std::vector<uint32_t>{0, 1, 1, 2, 1, 1}));
	return 0;
}
