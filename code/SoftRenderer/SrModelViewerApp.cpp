#include "StdAfx.h"
#include "SrModelViewerApp.h"
#include "SrScene.h"
#include "SrEntity.h"
#include "SrMaterial.h"
#include "SrCamera.h"

SrModelViewerApp::SrModelViewerApp(void)
{
	m_ssao = false;
	m_camdist = 10.0f;
	m_shade_mode = 0;
}


SrModelViewerApp::~SrModelViewerApp(void)
{
}

void SrModelViewerApp::OnInit()
{
	// ����Ⱦ����
	g_context->OpenFeature(eRFeature_MThreadRendering);
	g_context->OpenFeature(eRFeature_JitAA);
	g_context->OpenFeature(eRFeature_LinearFiltering);

	// ��������
	m_scene = new SrScene;
	gEnv->sceneMgr = m_scene;

	m_curr_ent = 0;

	float3 poszero = float3(0, 0, 0);
	Quat rotidtt = Quat::CreateIdentity();

	m_ent = m_scene->CreateEntity("object", "media/modelviewer/teapot.obj", "media/modelviewer/teapot.mtl", poszero, rotidtt);
	m_ents.push_back(m_ent);
	m_ent->SetScale(float3(40, 40, 40));
	
	//SwitchSSAO();

	// �������
	m_camera = m_scene->CreateCamera("cam0");
	m_camera->setPos(float3(0,0,-15));
	m_camera->setFov(68.0f);
	m_camera->Rotate( 0.1f, 0.3f );
	m_scene->PushCamera(m_camera);
	m_camdist = 15.0f;
	updateCam();

	// ���һ������
	SrLight* lt = gEnv->sceneMgr->AddLight();
	lt->diffuseColor = SR_ARGB_F( 255, 255, 239, 216 ) * 2.0f;
	lt->specularColor = SR_ARGB_F( 255, 255, 239, 216 );
	lt->worldPos = float3( 1000.f, 1000.f, -1000.f);
	lt->radius = 100.f;

	// set shademode -> 0
	m_shade_mode = 0;
	UpdateShader();
}

void SrModelViewerApp::OnUpdate()
{
	selectEnt(m_curr_ent);

	float3 move(0,0.5f * gEnv->timer->getElapsedTime(),0);
	m_ents[m_curr_ent]->RotateLocal(move);
	
	m_scene->Update();

	// dotCovarageǿ�ƹر�JITAA
	if ( g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) )
	{
		g_context->CloseFeature(eRFeature_JitAA);
	}

	// ��Ϣ���
	char buffer[255];
	int keyL = 15;
	int startxL = 70;
	int starty = 4 * g_context->height / 5;
}

void SrModelViewerApp::OnDestroy()
{
	// ɾ������
	m_ents.clear();
	m_ent = NULL;
	delete m_scene;
}

void SrModelViewerApp::SwitchSSAO()
{
	m_shade_mode++;
	m_shade_mode %= 4;
	UpdateShader();
}
void SrModelViewerApp::updateCam()
{
	m_camera->setPos( float3(0,0,0) );
	m_camera->Move( float3(0,0,-m_camdist));
}

void SrModelViewerApp::selectEnt(int index)
{
	for (int i=0; i < m_ents.size(); ++i)
	{
		m_ents[i]->SetVisible(false);
	}

	m_ents[index]->SetVisible(true);
}
void SrModelViewerApp::UpdateShader()
{
	switch( m_shade_mode )
	{
	case 0:
		for (uint32 i=0; i < m_ents[m_curr_ent]->getMaterialCount(); ++i)
		{
			m_ents[m_curr_ent]->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("default"));
		}
		break;
	case 1:
		for (uint32 i=0; i < m_ents[m_curr_ent]->getMaterialCount(); ++i)
		{
			m_ents[m_curr_ent]->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("fresnel"));
		}
		break;
	case 2:
		for (uint32 i=0; i < m_ents[m_curr_ent]->getMaterialCount(); ++i)
		{
			m_ents[m_curr_ent]->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("default_normal"));
		}
		break;
	case 3:
		for (uint32 i=0; i < m_ents[m_curr_ent]->getMaterialCount(); ++i)
		{
			m_ents[m_curr_ent]->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("skin"));
		}
		break;
	}
}