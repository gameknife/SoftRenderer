// SoftRenderer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#ifdef OS_WIN32

#include "SoftRenderApp.h"
#include "SrModelViewerApp.h"
#include "SrProfiler.h"

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

	app.SendEvent("set_model", "media\\sponza.obj");
	
	for(int i = 0 ; i < 100000; ++i)
	{
		app.Update();
		gEnv->logger->Log(gEnv->profiler->getProfileDataConsole());
	}
	//app.Destroy();

	return 0;
}

#endif


