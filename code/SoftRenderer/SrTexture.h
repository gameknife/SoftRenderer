/**
  @file SrTexture.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrTexture_h__
#define SrTexture_h__
#include "prerequisite.h"
#include "SrResource.h"
#include "SrTextureLayout.h"

enum ESrBitmapType
{
	eBt_file,
	eBt_internal,
};

struct SrTextureMipLevel
{
	uint32 width;
	uint32 height;
	uint32 tilesPerRow;
	size_t offset;
};

class SrTexture : public SrResource
{
public:
	SrTexture( const char* name )
		: SrResource(name, eRT_Texture), m_userData(NULL),
		  m_width(0), m_height(0), m_bpp(0), m_pitch(0),
		  m_data(NULL), m_storageSize(0), m_mortonOrder(false),
		  m_mortonTilesPerRow(0), m_texType(eBt_internal)
	{
		GtLog("[ResourceManager] Texture[%s] Created.", m_name.c_str());
	}
	virtual ~SrTexture(void)
	{

	}
	const uint8* getBuffer() const {return m_data;}
	int getBPP() const {return m_bpp;}
	int getWidth() const {return m_width;}
	int getHeight() const {return m_height;}  
	int getPitch() const {return m_pitch;}
	size_t getStorageSize() const {return m_storageSize;}
	bool isMortonOrder() const {return m_mortonOrder;}
	uint32 getMipCount() const {return static_cast<uint32>(m_mipLevels.size());}
	ESrBitmapType getTextureType() const {return m_texType;}

	void Apply(uint8 stage, uint8 samplertype) const
	{
		gEnv->renderer->SetTextureStage( this, stage);
	}

	SR_TEXTURE_FORCE_INLINE uint32 Get( const int2& p ) const
	{
		assert( p.x >=0 && p.x < m_width );
		assert( p.y >=0 && p.y < m_height );

		const uint32 x = static_cast<uint32>(p.x);
		const uint32 y = static_cast<uint32>(p.y);
		size_t texelIndex;
		if (m_mortonOrder)
		{
			// Fully expanded 8x8 tiled Morton address. Keeping this in the
			// sampling hot path avoids helper calls and exposes all constants
			// to the optimizer as shifts/masks.
			const size_t tileIndex =
				static_cast<size_t>(y >> 3) * m_mortonTilesPerRow + (x >> 3);
			const uint32 mortonIndex =
				(x & 1u) | ((y & 1u) << 1u) |
				((x & 2u) << 1u) | ((y & 2u) << 2u) |
				((x & 4u) << 2u) | ((y & 4u) << 3u);
			texelIndex = (tileIndex << 6) + mortonIndex;
		}
		else
		{
			texelIndex = static_cast<size_t>(y) * m_width + x;
		}

		const uint8* texel = m_data + texelIndex * static_cast<size_t>(m_bpp);
		const uint32 bgr =
			static_cast<uint32>(texel[0]) |
			(static_cast<uint32>(texel[1]) << 8u) |
			(static_cast<uint32>(texel[2]) << 16u);
		return m_bpp == 3
			? bgr | 0xff000000u
			: bgr | (static_cast<uint32>(texel[3]) << 24u);
	}

	void Set( const int2& p, uint32 color )
	{
		assert( p.x >=0 && p.x < m_width );
		assert( p.y >=0 && p.y < m_height );

		uint8* texel = m_data + GetTexelOffset(p.x, p.y);
		memcpy(texel, &color, m_bpp);
	}

	SR_TEXTURE_FORCE_INLINE uint32 Get(
		const float2& p, ESamplerFilter mode /*= eSF_Linear*/, float lod = 0.f) const
	{
		float u = p.x - floorf(p.x);
		float v = p.y - floorf(p.y);
		uint32 mipLevel = lod > 0.f ? static_cast<uint32>(lod + 0.5f) : 0;
		const uint32 mipCount = static_cast<uint32>(m_mipLevels.size());
		if (mipLevel >= mipCount)
			mipLevel = mipCount - 1;
		const SrTextureMipLevel& mip = m_mipLevels[mipLevel];

		if (mode == eSF_Nearest)
		{
			const uint32 x = static_cast<uint32>(u * mip.width) % mip.width;
			const uint32 y = static_cast<uint32>(v * mip.height) % mip.height;
			const size_t offset = GetTexelOffset(mipLevel, x, y);
			const uint8* texel = m_data + offset;
			return static_cast<uint32>(texel[0]) |
				(static_cast<uint32>(texel[1]) << 8u) |
				(static_cast<uint32>(texel[2]) << 16u) |
				(static_cast<uint32>(texel[3]) << 24u);
		}

		const float texelX = u * (mip.width - 1);
		const float texelY = v * (mip.height - 1);
		const uint32 x0 = static_cast<uint32>(texelX);
		const uint32 y0 = static_cast<uint32>(texelY);
		const uint32 x1 = x0 + 1 < mip.width ? x0 + 1 : x0;
		const uint32 y1 = y0 + 1 < mip.height ? y0 + 1 : y0;
		const float du = texelX - x0;
		const float dv = texelY - y0;

		size_t indices[4];
		if (m_mortonOrder)
		{
#define SR_MORTON_SAMPLE_INDEX(sampleX, sampleY) \
			((static_cast<size_t>((sampleY) >> 3) * mip.tilesPerRow + ((sampleX) >> 3)) << 6) + \
			(((sampleX) & 1u) | (((sampleY) & 1u) << 1u) | \
			(((sampleX) & 2u) << 1u) | (((sampleY) & 2u) << 2u) | \
			(((sampleX) & 4u) << 2u) | (((sampleY) & 4u) << 3u))
			indices[0] = SR_MORTON_SAMPLE_INDEX(x0, y0);
			indices[1] = SR_MORTON_SAMPLE_INDEX(x1, y0);
			indices[2] = SR_MORTON_SAMPLE_INDEX(x0, y1);
			indices[3] = SR_MORTON_SAMPLE_INDEX(x1, y1);
#undef SR_MORTON_SAMPLE_INDEX
		}
		else
		{
			indices[0] = static_cast<size_t>(y0) * mip.width + x0;
			indices[1] = static_cast<size_t>(y0) * mip.width + x1;
			indices[2] = static_cast<size_t>(y1) * mip.width + x0;
			indices[3] = static_cast<size_t>(y1) * mip.width + x1;
		}

		const uint8* texels[4] = {
			m_data + mip.offset + indices[0] * 4,
			m_data + mip.offset + indices[1] * 4,
			m_data + mip.offset + indices[2] * 4,
			m_data + mip.offset + indices[3] * 4
		};
		const float weights[4] = {
			(1.f - du) * (1.f - dv), du * (1.f - dv),
			(1.f - du) * dv, du * dv
		};
		uint32 result = 0;
		for (uint32 channel = 0; channel < 4; ++channel)
		{
			const float value =
				texels[0][channel] * weights[0] + texels[1][channel] * weights[1] +
				texels[2][channel] * weights[2] + texels[3][channel] * weights[3];
			result |= static_cast<uint32>(value) << (channel * 8);
		}
		return result;
	}

	mutable void*	m_userData;

protected:
	SrTexture(void);

	void ConfigureStorage(
		int width, int height, int bpp, bool mortonOrder, bool buildMipChain = false)
	{
		m_width = width;
		m_height = height;
		m_bpp = bpp;
		m_pitch = width * bpp;
		m_mortonOrder = mortonOrder;
		m_mipLevels.clear();
		m_storageSize = 0;
		uint32 mipWidth = static_cast<uint32>(width);
		uint32 mipHeight = static_cast<uint32>(height);
		do
		{
			SrTextureMipLevel mip;
			mip.width = mipWidth;
			mip.height = mipHeight;
			mip.tilesPerRow = mortonOrder
				? (mipWidth + SR_TEXTURE_MORTON_TILE_SIZE - 1) / SR_TEXTURE_MORTON_TILE_SIZE
				: 0;
			mip.offset = m_storageSize;
			m_mipLevels.push_back(mip);
			if (mortonOrder)
			{
				const size_t tileRows =
					(mipHeight + SR_TEXTURE_MORTON_TILE_SIZE - 1) /
					SR_TEXTURE_MORTON_TILE_SIZE;
				m_storageSize += static_cast<size_t>(mip.tilesPerRow) * tileRows *
					SR_TEXTURE_MORTON_TILE_TEXELS * static_cast<size_t>(bpp);
			}
			else
			{
				m_storageSize += static_cast<size_t>(mipWidth) * mipHeight * bpp;
			}
			mipWidth = (std::max)(1u, mipWidth >> 1);
			mipHeight = (std::max)(1u, mipHeight >> 1);
		}
		while (buildMipChain && (mipWidth != 1 || mipHeight != 1));
		if (buildMipChain && (m_mipLevels.back().width != 1 || m_mipLevels.back().height != 1))
		{
			SrTextureMipLevel mip = {1, 1, mortonOrder ? 1u : 0u, m_storageSize};
			m_mipLevels.push_back(mip);
			m_storageSize += mortonOrder
				? SR_TEXTURE_MORTON_TILE_TEXELS * static_cast<size_t>(bpp)
				: static_cast<size_t>(bpp);
		}
		m_mortonTilesPerRow = m_mipLevels[0].tilesPerRow;
	}

	SR_TEXTURE_FORCE_INLINE size_t GetTexelOffset(int x, int y) const
	{
		return GetTexelOffset(0, static_cast<uint32>(x), static_cast<uint32>(y));
	}

	SR_TEXTURE_FORCE_INLINE size_t GetTexelOffset(
		uint32 mipLevel, uint32 x, uint32 y) const
	{
		const SrTextureMipLevel& mip = m_mipLevels[mipLevel];
		if (m_mortonOrder)
		{
			return mip.offset + SrMortonTextureIndex(x, y, mip.tilesPerRow) *
				static_cast<size_t>(m_bpp);
		}
		return mip.offset +
			(static_cast<size_t>(y) * mip.width + x) * static_cast<size_t>(m_bpp);
	}

	void GenerateMipmaps();

	int m_width;
	int m_height;
	int m_bpp;
	int m_pitch;

	uint8*	m_data;
	size_t m_storageSize;
	bool m_mortonOrder;
	uint32 m_mortonTilesPerRow;
	std::vector<SrTextureMipLevel> m_mipLevels;
	ESrBitmapType m_texType;
	
};

#endif // SrTexture_h__

