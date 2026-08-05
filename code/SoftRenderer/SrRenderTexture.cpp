#include "stdafx.h"
#include "SrRenderTexture.h"
#include "mmgr/mmgr.h"

SrRenderTexture::SrRenderTexture( const char* name, int width, int height, int bpp ):SrTexture(name)
{
	// Render targets are exposed as linear frame buffers. Keep that write
	// contract; sampled file textures use the cache-friendly Morton layout.
	ConfigureStorage(width, height, bpp, false);
	m_texType = eBt_internal;
	// allocate buffer
	m_data = (uint8*)_mm_malloc_custom(m_storageSize, 16);
	memset(m_data, 0, m_storageSize);
}

SrRenderTexture::~SrRenderTexture(void)
{
	if (m_data)
	{
		_mm_free_custom(m_data);
	}
}
