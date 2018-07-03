// SoftRenderer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SoftRenderApp.h"
#include "SrSponzaApp.h"
#include "SrModelViewerApp.h"

#include "mmgr.h"


std::map<const void*, void*> m_align_pt_mapper;
std::string g_rootPath;

#ifdef OS_WIN32

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


