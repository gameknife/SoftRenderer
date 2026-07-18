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


SrBitmap::SrBitmap(const char* filename):SrTexture(filename)
{
	std::string realpath(filename);
	getMediaPath(realpath);

	SrMemFile bitmapfile;

	bitmapfile.Open(realpath.c_str());

	if (bitmapfile.IsOpen())
	{
		const char* start = bitmapfile.Data();

		// 获得文件头
		const SrBitmapFileHeader* header = reinterpret_cast<const SrBitmapFileHeader*>(start);
		const SrBitmapInfoHeader* info = reinterpret_cast<const SrBitmapInfoHeader*>(start + sizeof(SrBitmapFileHeader));

		// 初始化debug数据
		GtLog("BMP file openup[%s] width: %d | height: %d | bpp: %d | offset: %d | size: %d kb", getName(),
			info->width, info->height, info->bitCount, header->pixelOffset, header->size / 1024 );

		// 基本数据读取
		m_width = info->width;
		m_height = abs(info->height);
		m_bpp = info->bitCount / 8;
		m_pitch = -m_width * m_bpp * m_height / info->height;
		// copy file
		m_rawData = (uint8*)_mm_malloc_custom( m_bpp * m_width * m_height, 16);
		const uint8* colorstart = reinterpret_cast<const uint8*>(start + header->pixelOffset);

		memcpy(m_rawData, colorstart, m_bpp * m_width * m_height);

		if (m_pitch < 0)
		{
			m_data = m_rawData + m_bpp * m_width * m_height;
		}
		
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
