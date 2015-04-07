/**
  @file SrBitmap.cpp
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#include "StdAfx.h"
#include "SrBitmap.h"
#include "SrProfiler.h"

#include "mmgr/mmgr.h"


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
		tagBITMAPFILEHEADER* header = (tagBITMAPFILEHEADER*)start;
		tagBITMAPINFO* info = (tagBITMAPINFO*)(start + sizeof(tagBITMAPFILEHEADER));

		// 初始化debug数据
		GtLog("BMP file openup[%s] width: %d | height: %d | bpp: %d | offset: %d | size: %d kb", getName(),
			info->bmiHeader.biWidth,
			info->bmiHeader.biHeight,
			info->bmiHeader.biBitCount,
			header->bfOffBits,
			header->bfSize / 1024 );

		// 基本数据读取
		m_width = info->bmiHeader.biWidth;
		m_height = abs(info->bmiHeader.biHeight);
		m_bpp = info->bmiHeader.biBitCount / 8;
		m_pitch = -m_width * m_bpp * m_height / info->bmiHeader.biHeight;
		// copy file
		m_rawData = (uint8*)_mm_malloc_custom( m_bpp * m_width * m_height, 16);
		uint8* colorstart = (uint8*)(start + header->bfOffBits);

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

