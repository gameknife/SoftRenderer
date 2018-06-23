#include "StdAfx.h"
#include "SrRenderTexture.h"
#include "mmgr.h"

SrRenderTexture::SrRenderTexture( const char* name, int width, int height, int bpp ):SrTexture(name)
{
	m_width = width;
	m_height = height;
	m_bpp = bpp;
	m_data = NULL;
	m_texType = eBt_internal;
	m_pitch = bpp * width;
	// allocate buffer
	m_data = (uint8*)_mm_malloc_custom(width * height * bpp, 16);
	memset(m_data, 0, width * height * bpp );
}

SrRenderTexture::~SrRenderTexture(void)
{
	if (m_data)
	{
		_mm_free_custom(m_data);
	}
}
