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
	// ����Ⱦ����
	g_context->OpenFeature(eRFeature_MThreadRendering);
	g_context->OpenFeature(eRFeature_JitAA);
	g_context->OpenFeature(eRFeature_LinearFiltering);

	// ��������
	m_scene = new SrScene;
	gEnv->sceneMgr = m_scene;


	float3 poszero = float3(0, 0, 0);
	Quat rotidtt = Quat::CreateIdentity();

	// ����SPONZA
	m_ent = m_scene->CreateEntity("model1", "media/sponza.obj", "media/sponza.mtl", poszero, rotidtt);
	
	m_ent->SetScale(float3(2,2,2));
	SwitchSSAO();

	// �������
	m_camera = m_scene->CreateCamera("cam0");
	m_camera->setPos(float3(0,10,-45));
	m_camera->setFov(68.0f);
	m_scene->PushCamera(m_camera);

	// ���һ������
	SrLight* lt = gEnv->sceneMgr->AddLight();
	lt->diffuseColor = SR_ARGB_F( 255, 255, 239, 216 ) * 2.0f;
	lt->specularColor = SR_ARGB_F( 255, 255, 239, 216 );
	lt->worldPos = float3( 1000.f, 1000.f, -1000.f);
	lt->radius = 100.f;
}

void SrSponzaApp::OnUpdate()
{
	m_scene->Update();

	// dotCovarageǿ�ƹر�JITAA
	if ( g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) )
	{
		g_context->CloseFeature(eRFeature_JitAA);
	}
}

void SrSponzaApp::OnDestroy()
{
	// ɾ������
	delete m_scene;
}

void SrSponzaApp::SwitchSSAO()
{
	// SSAO�л�
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
