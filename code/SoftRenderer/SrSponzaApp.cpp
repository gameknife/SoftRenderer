#include "stdafx.h"
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
	if (!getenv("SR_DISABLE_JITAA"))
		g_context->OpenFeature(eRFeature_JitAA);
	if (!getenv("SR_DISABLE_LINEAR"))
		g_context->OpenFeature(eRFeature_LinearFiltering);
	const char* backend = getenv("SR_RENDERER_BACKEND");
	if (backend && _stricmp(backend, "tile") == 0)
		g_context->OpenFeature(eRFeature_TileBinning);

	// ��������
	m_scene = new SrScene;
	gEnv->sceneMgr = m_scene;

	// ����SPONZA
	m_ent = m_scene->CreateEntity("model1", "media\\sponza.obj", "media\\sponza.mtl");

	//m_ent = m_scene->CreateEntity("model1", "media\\prophet\\prophet.obj", "media\\prophet\\prophet.mtl");
	m_ent->SetScale(float3{{{2,2,2}}});
	if (!getenv("SR_DISABLE_SSAO"))
		SwitchSSAO();

	// �������
	m_camera = m_scene->CreateCamera("cam0");
	m_camera->setPos(float3{{{0,10,-45}}});
	m_camera->setFov(68.0f);
	m_scene->PushCamera(m_camera);

	// ���һ������
	SrLight* lt = gEnv->sceneMgr->AddLight();
	lt->diffuseColor = SR_ARGB_F( 255, 255, 239, 216 ) * 2.0f;
	lt->specularColor = SR_ARGB_F( 255, 255, 239, 216 );
	lt->worldPos = float3{{{ 1000.f, 1000.f, -1000.f}}};
	lt->radius = 100.f;

	// ��������豸�ص�
	gEnv->inputSys->AddListener(this);
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
	gEnv->inputSys->RemoveListener(this);
}

bool SrSponzaApp::OnInputEvent( const SInputEvent &event )
{
	static bool rotateMode = false;
	static float speed = 5.f;
	switch(event.keyId)
	{
	case  eKI_LShift:
		{
			if (event.state == eIS_Pressed)
			{
				speed = 20.f;
			}
			else if (event.state == eIS_Released)
			{
				speed = 5.f;
			}
		}
		break;
	case eKI_W:
		{
			if (event.state == eIS_Down)
			{
				m_camera->Move( float3{{{0,0,speed}}} * gEnv->timer->getElapsedTime() );
			}			
		}
		break;
	case eKI_S:
		{
			if (event.state == eIS_Down)
			{
				m_camera->Move( float3{{{0,0,-speed}}} * gEnv->timer->getElapsedTime() );
			}			
		}
		break;
	case eKI_A:
		{
			if (event.state == eIS_Down)
			{
				m_camera->Move( float3{{{-speed,0,0}}} * gEnv->timer->getElapsedTime() );
			}			
		}
		break;
	case eKI_D:
		{
			if (event.state == eIS_Down)
			{
				m_camera->Move( float3{{{speed,0,0}}} * gEnv->timer->getElapsedTime() );
			}			
		}
		break;
	case eKI_Mouse1:
		{
			if (event.state == eIS_Pressed)
			{
				rotateMode = true;
			}
			else if (event.state == eIS_Released)
			{
				rotateMode = false;
			}
		}
		break;
	case eKI_MouseX:
		{
			if (event.state == eIS_Changed && rotateMode)
			{
				m_camera->Rotate( 0, event.value * -0.001f );
			}
		}
		break;
	case eKI_MouseY:
		{
			if (event.state == eIS_Changed && rotateMode)
			{
				m_camera->Rotate( -event.value * 0.001f, 0 );
			}
		}
		break;
	case eKI_P:
		{
			if (event.state == eIS_Pressed)
			{
				SwitchSSAO();
			}
			break;
		}

	case eKI_K:
		{
			if (event.state == eIS_Pressed)
			{
				if (g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
				{
					g_context->CloseFeature(eRFeature_DotCoverageRendering);
				}
				else
				{
					g_context->OpenFeature(eRFeature_DotCoverageRendering);
				}
			}
		}
		break;
	case eKI_J:
		{
			if (event.state == eIS_Pressed)
			{
				if (g_context->IsFeatureEnable(eRFeature_JitAA))
				{
					g_context->CloseFeature(eRFeature_JitAA);
				}
				else
				{
					g_context->OpenFeature(eRFeature_JitAA);
				}
			}
		}
		break;
	case eKI_N:
		{
			if (event.state == eIS_Pressed)
			{
				if (g_context->IsFeatureEnable(eRFeature_LinearFiltering))
				{
					g_context->CloseFeature(eRFeature_LinearFiltering);
				}
				else
				{
					g_context->OpenFeature(eRFeature_LinearFiltering);
				}
			}
		}
		break;
	default:
		break;
	}
	return false;
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
