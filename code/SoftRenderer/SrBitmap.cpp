/**
  @file SrBitmap.cpp
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#include "stdafx.h"
#include "SrBitmap.h"
#include "SrProfiler.h"

#include "mmgr/mmgr.h"

#pragma pack(push, 1)
struct SrBitmapFileHeader
{
	uint16 type;
	uint32 size;
	uint16 reserved1;
	uint16 reserved2;
	uint32 pixelOffset;
};
struct SrBitmapInfoHeader
{
	uint32 size;
	int32_t width;
	int32_t height;
	uint16 planes;
	uint16 bitCount;
	uint32 compression;
	uint32 imageSize;
	int32_t xPixelsPerMeter;
	int32_t yPixelsPerMeter;
	uint32 colorsUsed;
	uint32 colorsImportant;
};
#pragma pack(pop)


SrBitmap::SrBitmap(const char* filename):SrTexture(filename), m_rawData(NULL)
{
	std::string realpath(filename);
	getMediaPath(realpath);

	SrMemFile bitmapfile;

	bitmapfile.Open(realpath.c_str());

	if (bitmapfile.IsOpen() && bitmapfile.Size() >=
		static_cast<int>(sizeof(SrBitmapFileHeader) + sizeof(SrBitmapInfoHeader)))
	{
		const char* start = bitmapfile.Data();

		// 获得文件头
		const SrBitmapFileHeader* header = reinterpret_cast<const SrBitmapFileHeader*>(start);
		const SrBitmapInfoHeader* info = reinterpret_cast<const SrBitmapInfoHeader*>(start + sizeof(SrBitmapFileHeader));

		// 初始化debug数据
		GtLog("BMP file openup[%s] width: %d | height: %d | bpp: %d | offset: %d | size: %d kb", getName(),
			info->width, info->height, info->bitCount, header->pixelOffset, header->size / 1024 );

		const int width = info->width;
		const int bpp = info->bitCount / 8;
		if (header->type != 0x4d42 || width <= 0 ||
			info->height == 0 || info->height == INT32_MIN ||
			(bpp != 3 && bpp != 4) || info->compression != 0 ||
			header->pixelOffset > static_cast<uint32>(bitmapfile.Size()))
		{
			GtLogError( "[ResourceManager] Bitmap[%s] has an unsupported or invalid format.", getName() );
			return;
		}

		const int height = abs(info->height);
		const size_t sourcePitch = (static_cast<size_t>(width) * bpp + 3u) & ~size_t(3u);
		const size_t sourceSize = sourcePitch * height;
		if (sourceSize > static_cast<size_t>(bitmapfile.Size() - header->pixelOffset))
		{
			GtLogError( "[ResourceManager] Bitmap[%s] has an unsupported or invalid format.", getName() );
			return;
		}

		// Normalize all sampled textures to BGRA32. This removes the 24/32-bit
		// branch and the three-byte address multiply from the sampling hot path.
		ConfigureStorage(width, height, 4, true, true);
		m_rawData = (uint8*)_mm_malloc_custom(m_storageSize, 16);
		if (!m_rawData)
		{
			GtLogError( "[ResourceManager] Bitmap[%s] allocation failed.", getName() );
			return;
		}
		m_data = m_rawData;
		memset(m_data, 0, m_storageSize);
		const uint8* colorstart = reinterpret_cast<const uint8*>(start + header->pixelOffset);

		// Positive-height BMPs store rows bottom-up. Reorient and swizzle once
		// during loading so sampling only needs the Morton address calculation.
		for (int y = 0; y < m_height; ++y)
		{
			const int sourceY = info->height > 0 ? m_height - 1 - y : y;
			const uint8* sourceRow = colorstart + static_cast<size_t>(sourceY) * sourcePitch;
			for (int x = 0; x < m_width; ++x)
			{
				const uint8* source = sourceRow + static_cast<size_t>(x) * bpp;
				uint8* target = m_data + GetTexelOffset(x, y);
				target[0] = source[0];
				target[1] = source[1];
				target[2] = source[2];
				target[3] = bpp == 4 ? source[3] : 0xff;
			}
		}
		GenerateMipmaps();
		
	}
	else
	{
		GtLogError( "[ResourceManager] Bitmap[%s] load failed.", getName() );
		m_rawData = 0;
		return;
	}
	
	m_texType = eBt_file;
}

SrBitmap::~SrBitmap(void)
{
	if (m_rawData)
	{
		_mm_free_custom(m_rawData);
	}
	
}
