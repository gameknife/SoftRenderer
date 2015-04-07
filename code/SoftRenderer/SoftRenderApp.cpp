#include "StdAfx.h"
#include "SoftRenderApp.h"
#include "resource.h"
#include <MMSystem.h>
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

// Forward declarations of functions included in this code module:

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
// 		switch (wmId)
// 		{
// 		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
//		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


SoftRenderApp::SoftRenderApp(void)
{
}


SoftRenderApp::~SoftRenderApp(void)
{
}

BOOL SoftRenderApp::Init( HINSTANCE hInstance)
{
	m_curr_task = 0;

	gEnv = new GlobalEnvironment;
	gEnv->logger = new SrLogger();

	GtLogInfo("///////////////////////////////////\n");
	GtLogInfo("SoftRenderer Init...\n\n");

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOFTRENDERER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;//MAKEINTRESOURCE(IDC_SOFTRENDERER);
	wcex.lpszClassName	= "SoftRenderer Window Class";
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SOFTRENDERER));

	RegisterClassEx(&wcex);


	m_hInst = hInstance; // Store instance handle in our global variable

	const int createWidth = 854;
	const int createHeight = 480;

	m_hWnd = CreateWindow("SoftRenderer Window Class", "SoftRenderer", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, createWidth, createHeight, NULL, NULL, hInstance, NULL);

	RECT realRect;
	GetClientRect(m_hWnd, &realRect);

	int width = realRect.right - realRect.left;
	int height = realRect.bottom - realRect.top;
	width = createWidth * 2 - width;
	height = createHeight * 2 - height;

	MoveWindow(m_hWnd, GetSystemMetrics(SM_CXSCREEN) / 2 - width / 2, GetSystemMetrics(SM_CYSCREEN) / 2 - height / 2, width, height, FALSE);

	GtLog("Window Created. width=%d height=%d", createWidth, createHeight);

	if (!m_hWnd)
	{
		return FALSE;
	}

	

	// 创建资源管理器
	GtLogInfo("Creating ResourceManger...");
	gEnv->resourceMgr = new SrResourceManager;
	GtLog("- Loading Shader List...");
	LoadShaderList();
	GtLog("- Creating Default Procedura Medias...");
	gEnv->resourceMgr->InitDefaultMedia();

	// 创建Render上下文
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

	// 显示窗口
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);

	// 切换焦点
	SetFocus(m_hWnd);
	SetForegroundWindow(m_hWnd);

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

	if( m_curr_task < m_tasks.size())
	{
		m_tasks[m_curr_task]->OnUpdate();
	}

	gEnv->profiler->Update();

	gEnv->renderer->EndFrame();

	gEnv->profiler->setEnd(ePe_FrameTime);

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
	for(;;)
	{
		MSG msg;

		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message != WM_QUIT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				
				break;
			}
		}
		else
		{
		
			if (!Update())
			{
				// need to clean the message loop (WM_QUIT might cause problems in the case of a restart)
				// another message loop might have WM_QUIT already so we cannot rely only on this 
				while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				break;
			}
		}

		//Update();
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
	std::string dir = "\\renderer\\";
	std::string path = "\\renderer\\*.dll";
	getMediaPath(dir);
	getMediaPath(path);

	WIN32_FIND_DATAA fd;
	HANDLE hff = FindFirstFileA(path.c_str(), &fd);
	BOOL bIsFind = TRUE;

	while(hff && bIsFind)
	{
		if(fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			// do not get into
		}
		else
		{
			std::string fullpath = dir + fd.cFileName;

			// load dll shaders
			HMODULE hDllHandle = 0;
			hDllHandle = LoadLibraryA( fullpath.c_str() );
			if (hDllHandle)
			{
				fnLoadRenderer fnLoad = (fnLoadRenderer)(GetProcAddress( hDllHandle, "LoadRenderer" ));

				IRenderer* renderer = fnLoad(gEnv);
				//renderer->InitRenderer(m_hWnd, g_context->width, g_context->height, 32);


				m_renderers.push_back(renderer);

				m_rendHandles.push_back(hDllHandle);
			}		
		}
		bIsFind = FindNextFileA(hff, &fd);
	}

	if ( m_renderers.empty() )
	{
		return false;
	}
	m_currRendererIndex = 0;
	gEnv->renderer = m_renderers[m_currRendererIndex];
	gEnv->renderer->InitRenderer(m_hWnd, g_context->width, g_context->height, 32);
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
