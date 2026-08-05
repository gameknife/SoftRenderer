#ifndef SrTextureLayout_h__
#define SrTextureLayout_h__

#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
#define SR_TEXTURE_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define SR_TEXTURE_FORCE_INLINE inline __attribute__((always_inline))
#else
#define SR_TEXTURE_FORCE_INLINE inline
#endif

// A small Morton-ordered tile keeps neighbouring 2D samples close in memory
// without padding a whole non-power-of-two texture to a square.
static constexpr std::uint32_t SR_TEXTURE_MORTON_TILE_SIZE = 8;
static constexpr std::uint32_t SR_TEXTURE_MORTON_TILE_TEXELS =
	SR_TEXTURE_MORTON_TILE_SIZE * SR_TEXTURE_MORTON_TILE_SIZE;

static SR_TEXTURE_FORCE_INLINE std::uint32_t SrMortonTileIndex(
	std::uint32_t x, std::uint32_t y)
{
	x &= SR_TEXTURE_MORTON_TILE_SIZE - 1;
	y &= SR_TEXTURE_MORTON_TILE_SIZE - 1;
	return (x & 1u) | ((y & 1u) << 1u) |
		((x & 2u) << 1u) | ((y & 2u) << 2u) |
		((x & 4u) << 2u) | ((y & 4u) << 3u);
}

static SR_TEXTURE_FORCE_INLINE std::size_t SrMortonTextureIndex(
	std::uint32_t x, std::uint32_t y, std::uint32_t tilesPerRow)
{
	const std::uint32_t tileX = x / SR_TEXTURE_MORTON_TILE_SIZE;
	const std::uint32_t tileY = y / SR_TEXTURE_MORTON_TILE_SIZE;
	const std::size_t tileIndex = static_cast<std::size_t>(tileY) * tilesPerRow + tileX;
	return tileIndex * SR_TEXTURE_MORTON_TILE_TEXELS + SrMortonTileIndex(x, y);
}

#endif // SrTextureLayout_h__
