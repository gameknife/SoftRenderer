/**
  @file SrBitmap.cpp
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#include "StdAfx.h"

#include "SrBitmap.h"
#include "SrProfiler.h"

#include "mmgr.h"



const int MIN_RGB = 0;
const int MAX_RGB = 255;
const int BMP_MAGIC_ID = 2;

struct bmpfile_magic
{
	unsigned char magic[BMP_MAGIC_ID];
};

struct bmpfile_header
{
	uint32_t file_size;
	uint16_t creator1;
	uint16_t creator2;
	uint32_t bmp_offset;
};

struct bmpfile_dib_info
{
	uint32_t header_size;
	int32_t width;
	int32_t height;
	uint16_t num_planes;
	uint16_t bits_per_pixel;
	uint32_t compression;
	uint32_t bmp_byte_size;
	int32_t hres;
	int32_t vres;
	uint32_t num_colors;
	uint32_t num_important_colors;
};


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
		bmpfile_magic* magic = (bmpfile_magic*)start;
		bmpfile_header* header = (bmpfile_header*)(start + sizeof(bmpfile_magic));
		bmpfile_dib_info* info = (bmpfile_dib_info*)(start + +sizeof(bmpfile_magic) + sizeof(bmpfile_header));

		// 初始化debug数据
		GtLog("BMP file openup[%s] width: %d | height: %d | bpp: %d | offset: %d | size: %d kb", getName(),
			info->width,
			info->height,
			info->bits_per_pixel,
			header->bmp_offset,
			header->file_size / 1024 );

		// 基本数据读取
		m_width = info->width;
		m_height = abs(info->height);
		m_bpp = info->bits_per_pixel / 8;
		m_pitch = -m_width * m_bpp * m_height / info->height;
		// copy file
		m_rawData = (uint8*)_mm_malloc_custom( m_bpp * m_width * m_height, 16);
		uint8* colorstart = (uint8*)(start + header->bmp_offset);

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

