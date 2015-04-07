#include "prerequisite.h"
#include "SrRenderContext.h"
#include "SrSoftRenderer.h"

SrRendContext* g_context;
GlobalEnvironment* gEnv;
SrSoftRenderer* g_renderer;

extern "C" __declspec(dllexport) IRenderer* LoadRenderer(GlobalEnvironment* pgEnv)
{
	gEnv = pgEnv;
	g_context = gEnv->context;

	g_renderer = new SrSoftRenderer();

	return g_renderer;
}

extern "C" __declspec(dllexport) void FreeRenderer()
{
	delete g_renderer;
}