/**
  @file SrPresenter_DDraw.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrPresenter_DDraw_h__
#define SrPresenter_DDraw_h__

#include "prerequisite.h"

class SrPresenter_DDraw
{
public:
	SrPresenter_DDraw(void);
	~SrPresenter_DDraw(void);

	bool InitPresenter(HWND hWnd, uint32 width, uint32 height, uint32 bpp);
	bool ShutdownPresenter();
	void Swap();

	uint8* Lock();
	void UnLock();

private:
	struct IDirectDraw7* m_dd;
	struct IDirectDrawSurface7* m_frontSurface;
	struct IDirectDrawSurface7* m_drawSurface;
};

#endif



