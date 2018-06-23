// SoftRenderer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SoftRenderApp.h"
#include "SrSponzaApp.h"
#include "SrModelViewerApp.h"

#include "mmgr.h"


std::map<const void*, void*> m_align_pt_mapper;

int main()
{
	SoftRenderApp app;

	app.RegisterTask(new SrModelViewerApp);
	app.Init();
	app.Update();


	// if (gEnv->output == false)
	// {
	// 	app->WriteToFile("testbmp.bmp");
	// 	gEnv->output = true;
	// }

	app.Destroy();

	return 0;
}
//
// SoftRenderApp* g_app;
//
// void initOfflineSystem()
// {
// 	g_app = new SoftRenderApp();
// 	g_app->RegisterTask(new SrModelViewerApp);
// 	g_app->Init();
// }
//
// void renderToBuffer()
// {
// 	g_app->Update();
// }
//
// void shutDown()
// {
// 	g_app->Destroy();
// }



