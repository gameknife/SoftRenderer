#include "StdAfx.h"
#include "SoftRenderApp.h"
#include "resource.h"
#include "SrMesh.h"
#include "SrProfiler.h"
#include "SrShader.h"
#include "SrLogger.h"
#include "SrSoftRenderer.h"

#include "mmgr.h"


// 全局变量
GlobalEnvironment* gEnv = NULL;
SrLogger* g_logger = NULL;
SrRendContext* g_context = NULL;
std::map<const void*, void*> m_align_pt_mapper;
std::string g_rootPath;




typedef IRenderer* (*fnLoadRenderer)(GlobalEnvironment* pgEnv);
typedef void (*fnFreeRenderer)();

SoftRenderApp::SoftRenderApp(void)
{
}


SoftRenderApp::~SoftRenderApp(void)
{
}

bool SoftRenderApp::Init()
{
	m_curr_task = 0;

	gEnv = new GlobalEnvironment();
	gEnv->logger = new SrLogger();

	GtLogInfo("///////////////////////////////////\n");
	GtLogInfo("SoftRenderer Init...\n\n");

	const int createWidth = 854;
	const int createHeight = 480;

	// ������Դ������
	GtLogInfo("Creating ResourceManger...");
	gEnv->resourceMgr = new SrResourceManager;
	GtLog("- Loading Shader List...");
	LoadShaderList();
	GtLog("- Creating Default Procedura Medias...");
	gEnv->resourceMgr->InitDefaultMedia();

	// ����Render������
	GtLogInfo("Creating Render Context...");
	g_context = new SrRendContext(createWidth, createHeight, 32);
	gEnv->context = g_context;

	InitRenderers();
	
	gEnv->timer = new SrTimer;
	gEnv->timer->Init();
	GtLogInfo("[Timer] initialized.");

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


	if (m_tasks.size() > 0)
	{
		m_tasks[m_curr_task]->OnInit();
	}

	return true;
}

static Quat g_rot = Quat::CreateIdentity();

bool SoftRenderApp::Update()
{

	gEnv->profiler->setBegin(ePe_FrameTime);

	gEnv->timer->Update();

	if (!gEnv->renderer)
	{
		return false;
	}	
	gEnv->renderer->BeginFrame();
	
	gEnv->renderer->HwClear();

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
		delete (*it);
	}

	ShutdownRenderers();

	delete g_context;
	delete gEnv->resourceMgr;

	if (gEnv->timer)
	{
		delete gEnv->timer;
	}

	if (gEnv->profiler)
	{
		delete gEnv->profiler;
	}

	delete gEnv->logger;

	delete gEnv;	
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
	m_renderer = new SrSoftRenderer();
	m_currRendererIndex = 0;
	gEnv->renderer = m_renderer;
	gEnv->renderer->InitRenderer(g_context->width, g_context->height, 32);

	return true;
}

void SoftRenderApp::ShutdownRenderers()
{
	m_renderer->ShutdownRenderer();
	delete m_renderer;
}

void SoftRenderApp::SendEvent(const char* event, const char* argument)
{
	if( m_curr_task < m_tasks.size())
	{
		m_tasks[m_curr_task]->OnEvent(event, argument);
	}
}