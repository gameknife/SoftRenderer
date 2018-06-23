// SoftRenderer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SoftRenderApp.h"
#include "SrSponzaApp.h"
#include "SrModelViewerApp.h"

#include "mmgr/mmgr.h"


std::map<const void*, void*> m_align_pt_mapper;

int main()
{
	SoftRenderApp app;

	app.RegisterTask(new SrModelViewerApp);
	app.Init();
	app.Update();
	app.Destroy();

	return 0;
}

