#include "StdAfx.h"
#include "SrSponzaApp.h"
#include "SrScene.h"
#include "SrEntity.h"
#include "SrMaterial.h"
#include "SrCamera.h"

SrSponzaApp::SrSponzaApp(void)
{
	m_ssao = false;
}


SrSponzaApp::~SrSponzaApp(void)
{
}

void SrSponzaApp::OnInit()
{
	// 打开渲染特性
	g_context->OpenFeature(eRFeature_MThreadRendering);
	g_context->OpenFeature(eRFeature_JitAA);
	g_context->OpenFeature(eRFeature_LinearFiltering);

	// 创建场景
	m_scene = new SrScene;
	gEnv->sceneMgr = m_scene;

	// 创建SPONZA
	m_ent = m_scene->CreateEntity("model1", "media\\sponza.obj", "media\\sponza.mtl");

	//m_ent = m_scene->CreateEntity("model1", "media\\prophet\\prophet.obj", "media\\prophet\\prophet.mtl");
	m_ent->SetScale(float3(2,2,2));
	SwitchSSAO();

	// 创建相机
	m_camera = m_scene->CreateCamera("cam0");
	m_camera->setPos(float3(0,10,-45));
	m_camera->setFov(68.0f);
	m_scene->PushCamera(m_camera);

	// 添加一个主光
	SrLight* lt = gEnv->sceneMgr->AddLight();
	lt->diffuseColor = SR_ARGB_F( 255, 255, 239, 216 ) * 2.0f;
	lt->specularColor = SR_ARGB_F( 255, 255, 239, 216 );
	lt->worldPos = float3( 1000.f, 1000.f, -1000.f);
	lt->radius = 100.f;
}

void SrSponzaApp::OnUpdate()
{
	m_scene->Update();

	// dotCovarage强制关闭JITAA
	if ( g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) )
	{
		g_context->CloseFeature(eRFeature_JitAA);
	}

	// 信息输出
	char buffer[255];
	int keyL = 15;
	int startxL = 70;
	int starty = 4 * g_context->height / 5;

	gEnv->renderer->DrawScreenText( "[Press P]", keyL, starty, 1, SR_UICOLOR_MAIN);
	sprintf_s( buffer, "SSAO: %s", m_ssao ? "on" : "off" );
	gEnv->renderer->DrawScreenText( buffer, startxL, starty, 1, SR_UICOLOR_NORMAL );

	gEnv->renderer->DrawScreenText( "[Press K]", keyL, starty += 10, 1, SR_UICOLOR_MAIN);
	sprintf_s( buffer, "DotCoverage: %s", g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) ? "on" : "off" );
	gEnv->renderer->DrawScreenText( buffer, startxL, starty, 1, SR_UICOLOR_NORMAL );

// 	gEnv->renderer->DrawScreenText( "[Press ~]", keyL, starty += 10, 1, SR_UICOLOR_MAIN);
// 	sprintf_s( buffer, "Renderer: %s", gEnv->renderer->m_rendererType == eRt_HardwareD3D9 ? "Hw_D3D9" : "Software" );
// 	gEnv->renderer->DrawScreenText( buffer, startxL, starty, 1, SR_UICOLOR_NORMAL );

	gEnv->renderer->DrawScreenText( "[CamCtrl]", keyL, starty += 10, 1, SR_UICOLOR_MAIN);
	sprintf_s( buffer, "WASD Move Cam | Mouse L+Drag Rotate Cam" );
	gEnv->renderer->DrawScreenText( buffer, startxL, starty, 1, SR_UICOLOR_NORMAL );
}

void SrSponzaApp::OnDestroy()
{
	// 删除场景
	delete m_scene;
}

void SrSponzaApp::SwitchSSAO()
{
	// SSAO切换
	if (m_ssao)
	{
		m_ssao = false;
		for (uint32 i=0; i < m_ent->getMaterialCount(); ++i)
		{
			m_ent->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("default"));
		}
	}
	else
	{
		m_ssao = true;
		for (uint32 i=0; i < m_ent->getMaterialCount(); ++i)
		{
			m_ent->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("default_normal"));
		}
	}

}
