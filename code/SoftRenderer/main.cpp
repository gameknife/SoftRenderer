// SoftRenderer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SoftRenderApp.h"
#include "SrSponzaApp.h"
#include "SrModelViewerApp.h"

#include "mmgr/mmgr.h"
#include "SrBitmap.h"


std::map<const void*, void*> m_align_pt_mapper;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	// mem leak detecting code...
	// Enable run-time memory check for debug builds.
#if (defined(DEBUG) || defined(_DEBUG))
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	
	SoftRenderApp app;

	app.RegisterTask(new SrModelViewerApp);
	app.Init(hInstance);
	app.Run();
	app.Destroy();

	return 0;
}
