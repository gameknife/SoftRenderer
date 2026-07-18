// SoftRenderer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SoftRenderApp.h"
#include "SrSponzaApp.h"
#include "SrModelViewerApp.h"
#include "SrPerformance.h"

#include "mmgr/mmgr.h"

static void SetOptionEnvironment(const char* commandLine, const char* option, const char* environment)
{
	const char* value = strstr(commandLine, option);
	if (!value)
		return;
	value += strlen(option);
	char buffer[1024];
	uint32 length = 0;
	while (value[length] && value[length] != ' ' && length + 1 < sizeof(buffer))
	{
		buffer[length] = value[length];
		++length;
	}
	buffer[length] = 0;
	_putenv_s(environment, buffer);
}

static bool HasCommandLineOption(const char* commandLine, const char* option)
{
	const size_t optionLength = strlen(option);
	const char* current = commandLine;
	while ((current = strstr(current, option)) != NULL)
	{
		const bool startsToken = current == commandLine || current[-1] == ' ' || current[-1] == '\t';
		const char next = current[optionLength];
		const bool endsToken = next == 0 || next == ' ' || next == '\t';
		if (startsToken && endsToken)
			return true;
		current += optionLength;
	}
	return false;
}

static void ApplyRendererCommandLine(const char* commandLine)
{
	// Prefer the pipelined tile renderer and keep the optional SSAO pass off
	// unless the caller explicitly requests different behavior.
	if (!getenv("SR_RENDERER_BACKEND")) _putenv_s("SR_RENDERER_BACKEND", "tile");
	if (!getenv("SR_DISABLE_SSAO")) _putenv_s("SR_DISABLE_SSAO", "1");

	if (HasCommandLineOption(commandLine, "--legacy")) _putenv_s("SR_RENDERER_BACKEND", "legacy");
	if (HasCommandLineOption(commandLine, "--tile")) _putenv_s("SR_RENDERER_BACKEND", "tile");
	if (HasCommandLineOption(commandLine, "--headless")) _putenv_s("SR_HEADLESS", "1");
	if (HasCommandLineOption(commandLine, "--no-jitaa")) _putenv_s("SR_DISABLE_JITAA", "1");
	if (HasCommandLineOption(commandLine, "--ssao")) _putenv_s("SR_DISABLE_SSAO", "");
	if (HasCommandLineOption(commandLine, "--no-ssao")) _putenv_s("SR_DISABLE_SSAO", "1");
	if (HasCommandLineOption(commandLine, "--no-ui")) _putenv_s("SR_DISABLE_UI", "1");
	if (HasCommandLineOption(commandLine, "--relaxed-ssao")) _putenv_s("SR_RELAXED_SSAO", "1");
	SetOptionEnvironment(commandLine, "--tile-size=", "SR_TILE_SIZE");
	SetOptionEnvironment(commandLine, "--workers=", "SR_WORKER_COUNT");
	SetOptionEnvironment(commandLine, "--frames=", "SR_FIXED_FRAMES");
	SetOptionEnvironment(commandLine, "--width=", "SR_WIDTH");
	SetOptionEnvironment(commandLine, "--height=", "SR_HEIGHT");
	SetOptionEnvironment(commandLine, "--scene=", "SR_SCENE");
	SetOptionEnvironment(commandLine, "--hash-output=", "SR_HASH_OUTPUT");
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	SrSetCurrentThreadName("SoftRenderer Main");
	PERFORMANCEAPI_INSTRUMENT("Application.Run");
	ApplyRendererCommandLine(lpCmdLine ? lpCmdLine : "");
	// mem leak detecting code...
	// Enable run-time memory check for debug builds.
#if (defined(DEBUG) || defined(_DEBUG))
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	
	SoftRenderApp app;

	const char* scene = getenv("SR_SCENE");
	if (scene && _stricmp(scene, "model") == 0)
	{
		app.RegisterTask(new SrModelViewerApp);
		app.RegisterTask(new SrSponzaApp);
	}
	else
	{
		app.RegisterTask(new SrSponzaApp);
		app.RegisterTask(new SrModelViewerApp);
	}

	app.Init(hInstance);
	app.Run();
	app.Destroy();

	return 0;
}
