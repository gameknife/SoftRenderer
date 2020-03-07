#include "stdafx.h"
#include "SrModelViewerApp.h"
#include "SrScene.h"
#include "SrEntity.h"
#include "SrMaterial.h"
#include "SrCamera.h"

SrModelViewerApp::SrModelViewerApp(void)
{
	m_ent = NULL;
	m_ssao = false;
	m_camdist = 10.0f;
	m_shade_mode = 0;
}


SrModelViewerApp::~SrModelViewerApp(void)
{
}

void SrModelViewerApp::OnInit()
{
	g_context->OpenFeature(eRFeature_MThreadRendering);
	//g_context->OpenFeature(eRFeature_JitAA);
	g_context->OpenFeature(eRFeature_LinearFiltering);
	g_context->OpenFeature(eRFeature_JitAA);

	m_scene = new SrScene;
	gEnv->sceneMgr = m_scene;
	
	m_camera = m_scene->CreateCamera("cam0");
	m_camera->setPos(float3::make(0,4,-20));
	m_camera->setFov(68.0f);
	m_camera->Rotate( 0.0f, 0.0f );
	m_scene->PushCamera(m_camera);
	m_camdist = 15.0f;
	updateCam();

	SrLight* lt = gEnv->sceneMgr->AddLight();
	lt->diffuseColor = SR_ARGB_F( 255, 255, 239, 216 ) * 2.0f;
	lt->specularColor = SR_ARGB_F( 255, 255, 239, 216 );
	lt->worldPos = float3::make( 1000.f, 1000.f, -1000.f);
	lt->radius = 100.f;

	m_shade_mode = 2;
	UpdateShader();
}

void SrModelViewerApp::OnUpdate()
{
	if(m_ent != NULL)
	{
		//float3 move(0,0.5f * gEnv->timer->getElapsedTime(),0);
		//m_ent->RotateLocal(move);
	}

	//m_camera->setPos( float3(0,150,-600) );
	m_camera->Move( float3::make(0,0,gEnv->timer->getElapsedTime() * 0.1f));
	if(m_camera->getPos().z > 10.0f)
	{
		m_camera->setPos(float3::make(0,4,-20));
	}
	
	m_scene->Update();

	if ( g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) )
	{
		g_context->CloseFeature(eRFeature_JitAA);
	}
}

void SrModelViewerApp::OnEvent(const char* event, const char* param)
{
	if( !strcmp(event, "set_model") )
	{
		if( m_ent != NULL )
		{
			m_scene->RemoveEntity(m_ent);
			m_ent = NULL;
		}

		if(strlen(param) > 4)
		{
			// make mtl file path
			const char* exstension = param + (strlen(param) - 4);
			if( !strcmp(exstension, ".obj") )
			{
				std::string mtlfile(param);
				mtlfile.replace( strlen(param) - 4, std::string::npos, ".mtl" );


				float3 poszero = float3::make(0, 0, 0);
				Quat rotidtt = Quat::CreateIdentity();

				m_ent = m_scene->CreateEntity("object", param, mtlfile.c_str(), poszero, rotidtt);
				m_ent->SetScale(float3::make(1, 1, 1));
				m_ent->SetVisible(true);

				UpdateShader();
			}
		}

	}
}

void SrModelViewerApp::OnDestroy()
{
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
	//m_camera->setPos( float3(0,150,-600) );
	//m_camera->Move( float3(0,0,-m_camdist));
}

void SrModelViewerApp::UpdateShader()
{
	if(m_ent != NULL)
	{
		switch( m_shade_mode )
		{
		case 0:
			for (uint32 i=0; i < m_ent->getMaterialCount(); ++i)
			{
				m_ent->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("default"));
			}
			break;
		case 1:
			for (uint32 i=0; i < m_ent->getMaterialCount(); ++i)
			{
				m_ent->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("fresnel"));
			}
			break;
		case 2:
			for (uint32 i=0; i < m_ent->getMaterialCount(); ++i)
			{
				m_ent->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("default_normal"));
			}
			break;
		case 3:
			for (uint32 i=0; i < m_ent->getMaterialCount(); ++i)
			{
				m_ent->getMaterial(i)->SetShader(gEnv->resourceMgr->GetShader("skin"));
			}
			break;
		}
	}
}