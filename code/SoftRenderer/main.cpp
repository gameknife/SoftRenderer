// SoftRenderer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"


#ifdef OS_WIN32

#include "SoftRenderApp.h"
#include "SrModelViewerApp.h"

#include "mmgr.h"

int main()
{
	char buffer[MAX_PATH];
	char* strLastSlash = NULL;
	GetModuleFileName(NULL, buffer, MAX_PATH);
	buffer[MAX_PATH - 1] = 0;

	strLastSlash = strrchr(buffer, '\\');
	if (strLastSlash)
	{
		*(strLastSlash) = '\0';
	}
	strLastSlash = strrchr(buffer, '\\');
	if (strLastSlash)
	{
		*(strLastSlash) = '\0';
	}
	srSetRootPath(buffer);

	SoftRenderApp app;

	app.RegisterTask(new SrModelViewerApp);
	app.Init();
	app.Update();
	app.Update();
	app.Update();
	app.Update();
	app.Destroy();

	return 0;
}

#endif


