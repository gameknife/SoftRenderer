#include "stdafx.h"
#include "SrTexture.h"
#include "SrProfiler.h"

#include "mmgr/mmgr.h"

void SrTexture::GenerateMipmaps()
{
	assert(m_bpp == 4);
	for (uint32 level = 1; level < m_mipLevels.size(); ++level)
	{
		const SrTextureMipLevel& sourceMip = m_mipLevels[level - 1];
		const SrTextureMipLevel& targetMip = m_mipLevels[level];
		for (uint32 y = 0; y < targetMip.height; ++y)
			for (uint32 x = 0; x < targetMip.width; ++x)
			{
				uint32 channelSums[4] = {0, 0, 0, 0};
				uint32 sampleCount = 0;
				for (uint32 oy = 0; oy < 2; ++oy)
					for (uint32 ox = 0; ox < 2; ++ox)
					{
						const uint32 sourceX = x * 2 + ox;
						const uint32 sourceY = y * 2 + oy;
						if (sourceX >= sourceMip.width || sourceY >= sourceMip.height)
							continue;
						const uint8* source =
							m_data + GetTexelOffset(level - 1, sourceX, sourceY);
						for (uint32 channel = 0; channel < 4; ++channel)
							channelSums[channel] += source[channel];
						++sampleCount;
					}

				uint8* target = m_data + GetTexelOffset(level, x, y);
				for (uint32 channel = 0; channel < 4; ++channel)
					target[channel] = static_cast<uint8>(
						(channelSums[channel] + sampleCount / 2) / sampleCount);
			}
	}
}
