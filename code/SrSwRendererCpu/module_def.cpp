#include "prerequisite.h"
#include "SrRenderContext.h"
#include "SrSoftRenderer.h"

SrRendContext* g_context;
GlobalEnvironment* gEnv;
SrSoftRenderer* g_renderer;

extern "C" SR_EXPORT IRenderer* LoadRenderer(GlobalEnvironment* pgEnv)
{
	gEnv = pgEnv;
	g_context = gEnv->context;

	g_renderer = new SrSoftRenderer();

	return g_renderer;
}

extern "C" SR_EXPORT void FreeRenderer()
{
	delete g_renderer;
}
