#include "stdafx.h"
#include "SrPerformance.h"
#include "SoftRenderApp.h"
#include <MiniFB.h>
#include <filesystem>
#include "SrMesh.h"
#include "InputManager.h"
#include "SrProfiler.h"
#include "SrShader.h"
#include "SrLogger.h"

#include "mmgr/mmgr.h"

GlobalEnvironment* gEnv = NULL;
SrLogger* g_logger = NULL;
SrRendContext* g_context = NULL;

typedef IRenderer* (*fnLoadRenderer)(GlobalEnvironment* pgEnv);
typedef void (*fnFreeRenderer)();

// MiniFB destroys its window data before returning MFB_STATE_EXIT on macOS.
// Keep a single authoritative live pointer so renderer shutdown cannot present
// to (or close) the stale HWND retained by the renderer and input manager.
static mfb_window* g_miniFBWindow = NULL;

static bool PresentMiniFB(HWND window, const void* buffer)
{
	if (!window)
		return true;

	mfb_window* liveWindow = g_miniFBWindow;
	if (reinterpret_cast<mfb_window*>(window) != liveWindow)
		return false;

	const mfb_update_state state = mfb_update(liveWindow, const_cast<void*>(buffer));
	if (state < 0)
		g_miniFBWindow = NULL;
	return state >= 0;
}

SoftRenderApp::SoftRenderApp(void)
	: m_hWnd(NULL), m_currRendererIndex(0), m_curr_task(0), m_lastTitleUpdate(0.f)
{
}


SoftRenderApp::~SoftRenderApp(void)
{
}

BOOL SoftRenderApp::Init()
{
	m_curr_task = 0;

	gEnv = new GlobalEnvironment;
	memset(gEnv, 0, sizeof(GlobalEnvironment));
	gEnv->logger = new SrLogger();
	gEnv->present = PresentMiniFB;

	GtLogInfo("///////////////////////////////////\n");
	GtLogInfo("SoftRenderer Init...\n\n");

	const int createWidth = getenv("SR_WIDTH") ? (std::max)(1, atoi(getenv("SR_WIDTH"))) : 854;
	const int createHeight = getenv("SR_HEIGHT") ? (std::max)(1, atoi(getenv("SR_HEIGHT"))) : 480;
	g_miniFBWindow = getenv("SR_HEADLESS") ? NULL : mfb_open("SoftRenderer", createWidth, createHeight);
	m_hWnd = reinterpret_cast<HWND>(g_miniFBWindow);

	GtLog("Window Created. width=%d height=%d", createWidth, createHeight);

	if (!m_hWnd && !getenv("SR_HEADLESS"))
	{
		return FALSE;
	}

	

	// 锟斤拷锟斤拷锟斤拷源锟斤拷锟斤拷锟斤拷
	GtLogInfo("Creating ResourceManger...");
	gEnv->resourceMgr = new SrResourceManager;
	GtLog("- Loading Shader List...");
	LoadShaderList();
	GtLog("- Creating Default Procedura Medias...");
	gEnv->resourceMgr->InitDefaultMedia();

	// 锟斤拷锟斤拷Render锟斤拷锟斤拷锟斤拷
	GtLogInfo("Creating Render Context...");
	g_context = new SrRendContext(createWidth, createHeight, 32);
	gEnv->context = g_context;

	InitRenderers();
	
	gEnv->timer = new SrTimer;
	gEnv->timer->Init();
	GtLogInfo("[Timer] initialized.");

	gEnv->inputSys = new SrInputManager;
	gEnv->inputSys->Init(m_hWnd);
	gEnv->inputSys->AddListener(this);
	GtLogInfo("[InputManager] initialized.");

	gEnv->profiler = new SrProfiler;
	GtLogInfo("[Profiler] initialized.");

	GtLogInfo("Math Lib Struct Size:");
#define OUTPUT_SIZE( x ) \
	GtLog("-"#x" size: %d", sizeof(x));

	OUTPUT_SIZE( float2 )
	OUTPUT_SIZE( float3 )
	OUTPUT_SIZE( float4 )
	OUTPUT_SIZE( float33 )
	OUTPUT_SIZE( float44 )
	OUTPUT_SIZE( Quat )


	GtLogInfo("Base system initialized.");
	GtLogInfo("///////////////////////////////////\n\n");
// 	SrApps::iterator it = m_tasks.begin();
// 	for (; it != m_tasks.end(); ++it)
// 	{
// 		(*it)->OnInit();
// 	}

	if (m_tasks.size() > 0)
	{
		m_tasks[m_curr_task]->OnInit();
	}

	return TRUE;
}

static Quat g_rot = Quat::CreateIdentity();

bool SoftRenderApp::Update()
{
	PERFORMANCEAPI_INSTRUMENT("Frame");

	gEnv->profiler->setBegin(ePe_FrameTime);

	gEnv->timer->Update();

	gEnv->inputSys->Update();

	if (!gEnv->renderer)
	{
		return false;
	}	
	gEnv->renderer->BeginFrame();
	
	gEnv->renderer->HwClear();

// 	SrApps::iterator it = m_tasks.begin();
// 	for (; it != m_tasks.end(); ++it)
// 	{
// 		(*it)->OnUpdate();
// 	}

	if (m_curr_task >= 0 && static_cast<size_t>(m_curr_task) < m_tasks.size())
	{
		PERFORMANCEAPI_INSTRUMENT("Main.SceneUpdate");
		m_tasks[m_curr_task]->OnUpdate();
	}

	gEnv->renderer->EndFrame();

	gEnv->profiler->setEnd(ePe_FrameTime);

	// Updating the native title every frame can perturb the timings we are
	// trying to display, so refresh it at a human-readable rate instead.
	const float now = gEnv->timer->getRealTime();
	if (g_miniFBWindow && now - m_lastTitleUpdate >= 0.25f)
	{
		const float frameMs = gEnv->profiler->getAverageTime(ePe_FrameTime);
		const float fps = frameMs > 0.001f ? 1000.f / frameMs : 0.f;
		char title[512];
		std::snprintf(title, sizeof(title),
			"SoftRenderer | %s | %.1f FPS  %.2f ms | Flush %.2f  VS %.2f  RS %.2f  PS %.2f ms | %dx%d | %d workers",
			SrGetCpuModelName().c_str(), fps, frameMs,
			gEnv->profiler->getAverageTime(ePe_FlushTime),
			gEnv->profiler->getAverageTime(ePe_VertexShaderTime),
			gEnv->profiler->getAverageTime(ePe_RasterizeShaderTime),
			gEnv->profiler->getAverageTime(ePe_PixelShaderTime),
			g_context->width, g_context->height,
			(std::max)(0, g_context->processorNum - 1));
		mfb_set_title(g_miniFBWindow, title);
		m_lastTitleUpdate = now;
	}

	return true;
}

void SoftRenderApp::Destroy()
{
	SrApps::iterator it = m_tasks.begin();
	for (; it != m_tasks.end(); ++it)
	{
		//(*it)->OnDestroy();

		// should delete task by me!
		delete (*it);
	}

	ShutdownRenderers();
	mfb_window* liveWindow = g_miniFBWindow;
	g_miniFBWindow = NULL;
	m_hWnd = NULL;
	if (liveWindow)
	{
		// mfb_close only requests closure. Pump once so MiniFB can release its
		// platform data while this pointer is still known to be valid.
		mfb_close(liveWindow);
		mfb_update_events(liveWindow);
	}

	delete g_context;
	delete gEnv->resourceMgr;

	if (gEnv->timer)
	{
		delete gEnv->timer;
	}

	if (gEnv->inputSys)
	{
		gEnv->inputSys->Destroy();
		delete gEnv->inputSys;
	}

	if (gEnv->profiler)
	{
		delete gEnv->profiler;
	}

	delete gEnv->logger;

	delete gEnv;	
}

void SoftRenderApp::Run()
{
	const uint32 fixedFrames = getenv("SR_FIXED_FRAMES") ? static_cast<uint32>(atoi(getenv("SR_FIXED_FRAMES"))) : 0;
	uint32 renderedFrames = 0;
	for(;;)
	{
		// A present call can observe Escape/window-close and destroy MiniFB's
		// platform data. Do not poll the stale copy retained in m_hWnd.
		if (m_hWnd && !g_miniFBWindow)
		{
			m_hWnd = NULL;
			break;
		}
		if (g_miniFBWindow)
		{
			mfb_window* liveWindow = g_miniFBWindow;
			if (mfb_update_events(liveWindow) < 0)
			{
				g_miniFBWindow = NULL;
				m_hWnd = NULL;
				break;
			}
		}
		if (!Update()) break;
		++renderedFrames;
		if (fixedFrames && renderedFrames >= fixedFrames) break;
	}
}

void SoftRenderApp::RegisterTask( SrAppFramework* task )
{
	SrApps::iterator it = m_tasks.begin();
	for (; it != m_tasks.end(); ++it)
	{
		if ( *it == task)
		{
			return;
		}
	}

	m_tasks.push_back(task);
}

void SoftRenderApp::UnRegisterTasks()
{
	SrApps::iterator it = m_tasks.begin();
	for (; it != m_tasks.end(); ++it)
	{
		delete (*it);
	}
}

bool SoftRenderApp::OnInputEvent( const SInputEvent &event )
{
	switch(event.keyId)
	{
		case eKI_Tilde:
			{
// temporary mute hw renderer [4/6/2015 gameKnife]
// 				if (event.state == eIS_Pressed)
// 				{
// 					m_currRendererIndex++;
// 					if (m_currRendererIndex >= m_renderers.size())
// 					{
// 						m_currRendererIndex = 0;
// 					}
// 
// 					gEnv->renderer->ShutdownRenderer();
// 
// 					gEnv->renderer = m_renderers[m_currRendererIndex];
// 
// 					gEnv->renderer->InitRenderer(m_hWnd, g_context->width, g_context->height, 32);
// 				}
			}
			break;

		case eKI_Up:
			if (event.state == eIS_Pressed)
			{
				m_tasks[m_curr_task]->OnDestroy();

				m_curr_task++;
				m_curr_task %= m_tasks.size();

				m_tasks[m_curr_task]->OnInit();
			}



			break;
		case eKI_Down:
			if (event.state == eIS_Pressed)
			{
				m_tasks[m_curr_task]->OnDestroy();

				m_curr_task--;
				if (m_curr_task < 0)
				{
					m_curr_task = m_tasks.size() - 1;
				}

				m_tasks[m_curr_task]->OnInit();
			}

			break;

	}
	return false;
}

void SoftRenderApp::LoadShaderList()
{
	gEnv->resourceMgr->AddShader( new SrShader( "default", eVd_F4F4F4  ));
	gEnv->resourceMgr->AddShader( new SrShader( "skin", eVd_F4F4F4F4U4  ));
	gEnv->resourceMgr->AddShader( new SrShader( "hair", eVd_F4F4F4  ));
	gEnv->resourceMgr->AddShader( new SrShader( "fresnel", eVd_F4F4F4  ));

	gEnv->resourceMgr->AddShader( new SrShader( "default_normal", eVd_F4F4F4  ));
	//gEnv->resourceMgr->AddShader( new SrShader( "default_normal_ssao", eVd_F4F4F4  ));
}

bool SoftRenderApp::InitRenderers()
{
	std::string dir = "renderer";
	getMediaPath(dir);
	if (std::filesystem::exists(dir))
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir))
		{
			if (!entry.is_regular_file() || entry.path().filename().string().find("SrSwRendererCpu") == std::string::npos) continue;
			const std::string rendererPath = entry.path().string();
			HMODULE handle = LoadLibraryA(rendererPath.c_str());
			if (!handle) { GtLogError("Unable to load renderer: %s", SrGetLastDynamicLibraryError()); continue; }
			fnLoadRenderer load = reinterpret_cast<fnLoadRenderer>(GetProcAddress(handle, "LoadRenderer"));
			if (!load) { FreeLibrary(handle); continue; }
			m_renderers.push_back(load(gEnv));
			m_rendHandles.push_back(handle);
		}
	}

	if ( m_renderers.empty() )
	{
		return false;
	}
	m_currRendererIndex = 0;
	gEnv->renderer = m_renderers[m_currRendererIndex];
	return gEnv->renderer->InitRenderer(m_hWnd, g_context->width, g_context->height, 32);
}

void SoftRenderApp::ShutdownRenderers()
{
	for (uint32 i=0; i < m_rendHandles.size(); ++i)
	{
		m_renderers[i]->ShutdownRenderer();

		fnFreeRenderer fnFree = (fnFreeRenderer)(GetProcAddress( m_rendHandles[i], "FreeRenderer" ));

		fnFree();

		FreeLibrary( m_rendHandles[i] );
	}
}
