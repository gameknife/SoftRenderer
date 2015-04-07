#include "SrPresenter_DDraw.h"
#include <ddraw.h>

SrPresenter_DDraw::SrPresenter_DDraw():m_dd(NULL),
	m_frontSurface(NULL),
	m_drawSurface(NULL)
{

}


SrPresenter_DDraw::~SrPresenter_DDraw(void)
{
}

bool SrPresenter_DDraw::InitPresenter( HWND hWnd, uint32 width, uint32 height, uint32 bpp )
{
	// 创建ddraw对象
	if (FAILED(DirectDrawCreateEx(NULL, (void **)&m_dd, IID_IDirectDraw7, NULL)))
		return 0;

	// 窗口模式（设置协作级别）
	m_dd->SetCooperativeLevel(hWnd, DDSCL_NORMAL);

	//m_dd->SetDisplayMode( width, height, bpp, 0, 0 );

	DDSURFACEDESC2 ddDesc;
	memset( &ddDesc, 0, sizeof(ddDesc) );

	ddDesc.dwSize = sizeof(ddDesc);
	ddDesc.dwFlags = DDSD_CAPS;
	ddDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	ddDesc.dwBackBufferCount = 1;

	if ( FAILED( m_dd->CreateSurface( &ddDesc, &m_frontSurface, NULL ) ) )
	{
		return 0;
	}

	ddDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT| DDSD_CAPS;
	ddDesc.dwWidth = width;
	ddDesc.dwHeight = height;
	ddDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddDesc.ddpfPixelFormat.dwRGBBitCount = 32;
	ddDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

	if ( FAILED( m_dd->CreateSurface( &ddDesc, &m_drawSurface, NULL ) ) )
	{
		return 0;
	}

	return true;
}


bool SrPresenter_DDraw::ShutdownPresenter()
{
	SAFE_RELEASE( m_drawSurface );
	SAFE_RELEASE( m_dd );

	return true;
}

void SrPresenter_DDraw::Swap()
{
	if (m_frontSurface)
	{
		m_frontSurface->Flip( NULL, 0 );
	}
}

uint8* SrPresenter_DDraw::Lock()
{
	DDSURFACEDESC2 ddDesc2;
	memset(&ddDesc2, 0, sizeof(ddDesc2));
	m_drawSurface->Lock(NULL, &ddDesc2, DDLOCK_WRITEONLY, NULL);

	return (uint8*)ddDesc2.lpSurface;
}

void SrPresenter_DDraw::UnLock()
{
	m_drawSurface->Unlock(NULL);
}
