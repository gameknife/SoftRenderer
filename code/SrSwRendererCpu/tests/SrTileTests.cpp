#include "SrTileTypes.h"
#include "SrTileRenderer.h"
#include "RendererBase.h"
#include "SrTexture.h"
#include "SrTextureLayout.h"
#include <cassert>
#include <vector>

GlobalEnvironment g_testEnvironment = {};
GlobalEnvironment* gEnv = &g_testEnvironment;
static_assert(sizeof(SrFragment) == 80, "Mip footprint must not grow the G-buffer fragment");

class SrTestTexture : public SrTexture
{
public:
	SrTestTexture(uint32 width, uint32 height)
		: SrTexture("$mip_test")
	{
		ConfigureStorage(width, height, 4, true, true);
		m_storage.resize(m_storageSize, 0);
		m_data = m_storage.data();
	}

	void BuildMipmaps()
	{
		GenerateMipmaps();
	}

private:
	std::vector<uint8> m_storage;
};

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

static void CheckMortonTextureLayout(uint32_t width, uint32_t height)
{
	const uint32_t tilesPerRow =
		(width + SR_TEXTURE_MORTON_TILE_SIZE - 1) / SR_TEXTURE_MORTON_TILE_SIZE;
	const uint32_t tileRows =
		(height + SR_TEXTURE_MORTON_TILE_SIZE - 1) / SR_TEXTURE_MORTON_TILE_SIZE;
	std::vector<unsigned char> occupied(
		static_cast<size_t>(tilesPerRow) * tileRows * SR_TEXTURE_MORTON_TILE_TEXELS, 0);
	for (uint32_t y = 0; y < height; ++y)
		for (uint32_t x = 0; x < width; ++x)
		{
			const size_t index = SrMortonTextureIndex(x, y, tilesPerRow);
			assert(index < occupied.size());
			assert(occupied[index] == 0);
			occupied[index] = 1;
		}
}

static void CheckMipChain()
{
	SrTestTexture texture(4, 4);
	const uint32 colors[4] = {
		0xff000000u, 0xff202020u, 0xff404040u, 0xff606060u
	};
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x)
			texture.Set(int2(x, y), colors[(y >= 2) * 2 + (x >= 2)]);
	texture.BuildMipmaps();

	assert(texture.getBPP() == 4);
	assert(texture.getMipCount() == 3);
	assert(texture.Get(float2{{{0.1f, 0.1f}}}, eSF_Nearest, 1.f) == colors[0]);
	assert(texture.Get(float2{{{0.6f, 0.1f}}}, eSF_Nearest, 1.f) == colors[1]);
	assert(texture.Get(float2{{{0.1f, 0.6f}}}, eSF_Nearest, 1.f) == colors[2]);
	assert(texture.Get(float2{{{0.6f, 0.6f}}}, eSF_Nearest, 1.f) == colors[3]);
	assert(texture.Get(float2{{{0.f, 0.f}}}, eSF_Nearest, 2.f) == 0xff303030u);

	SrTestTexture oddTexture(7, 5);
	assert(oddTexture.getMipCount() == 3);
}

int main()
{
	const uint32_t resolutions[][2] = {{1,1}, {31,17}, {32,32}, {33,33}, {854,480}, {1280,720}, {1920,1080}};
	const uint32_t sizes[] = {8,16,32,64};
	for (size_t r = 0; r < sizeof(resolutions) / sizeof(resolutions[0]); ++r)
		for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s)
			CheckLayout(resolutions[r][0], resolutions[r][1], sizes[s]);

	const uint32_t textureResolutions[][2] = {
		{1,1}, {7,5}, {8,8}, {9,17}, {64,256}, {255,127}, {1920,1080}
	};
	for (size_t i = 0; i < sizeof(textureResolutions) / sizeof(textureResolutions[0]); ++i)
		CheckMortonTextureLayout(textureResolutions[i][0], textureResolutions[i][1]);
	assert(SrMortonTextureIndex(0, 0, 1) == 0);
	assert(SrMortonTextureIndex(1, 0, 1) == 1);
	assert(SrMortonTextureIndex(0, 1, 1) == 2);
	assert(SrMortonTextureIndex(1, 1, 1) == 3);
	CheckMipChain();

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
