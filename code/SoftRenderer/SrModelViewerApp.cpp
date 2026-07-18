#include "stdafx.h"
#include "SrModelViewerApp.h"
#include "SrScene.h"
#include "SrEntity.h"
#include "SrMaterial.h"
#include "SrCamera.h"

#include <filesystem>

inline void enum_all_files_in_folder(const char* root_path, std::vector<std::string>& result, bool inc_sub_folders = false)
{
	if (!root_path || !std::filesystem::exists(root_path)) return;
	if (inc_sub_folders)
		for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root_path))
			if (entry.is_regular_file()) result.push_back(entry.path().filename().string());
	else
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root_path))
			if (entry.is_regular_file()) result.push_back(entry.path().filename().string());
}

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

	m_curr_ent = 0;

	// ����SPONZA
	
	std::string dir = "\\media\\modelviewer\\";
	getMediaPath(dir);

	//m_ent = m_scene->CreateEntity("model1", "media\\head.obj", "media\\head.mtl");
	std::vector<std::string> result;
	enum_all_files_in_folder( dir.c_str(), result ,false );

	for (size_t i = 0; i < result.size(); ++i)
	{
		//if ( result[i] )
		{
			char file[MAX_PATH];
			strcpy( file, result[i].c_str());

			char* ext = strrchr( file, '.' );
			if (ext && !_stricmp( ext, ".obj" ))
			{
				*ext = 0;

				std::string filename = "media\\modelviewer\\";
				std::string objfile = filename + file + ".obj";
				std::string mtlfile = filename + file + ".mtl";
				m_ent = m_scene->CreateEntity(file, objfile.c_str(), mtlfile.c_str());
				m_ents.push_back( m_ent );
				m_ent->SetScale(float3(40,40,40));
			}
		}

	}

	
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

	// ��������豸�ص�
	gEnv->inputSys->AddListener(this);
}

void SrModelViewerApp::OnUpdate()
{
	selectEnt(m_curr_ent);
	m_scene->Update();

	// dotCovarageǿ�ƹر�JITAA
	if ( g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) )
	{
		g_context->CloseFeature(eRFeature_JitAA);
	}

}

void SrModelViewerApp::OnDestroy()
{
	// ɾ������
	m_ents.clear();
	m_ent = NULL;
	delete m_scene;
	gEnv->inputSys->RemoveListener(this);
}

bool SrModelViewerApp::OnInputEvent( const SInputEvent &event )
{
	static bool rotateMode = false;
	static float speed = 5.f;
	static bool shiftMode = false;
	static bool altMode = false;
	static bool ctrlMode = false;
	switch(event.keyId)
	{
	case  eKI_LShift:
		{
			if (event.state == eIS_Pressed)
			{
				speed = 20.f;
				shiftMode = true;
			}
			else if (event.state == eIS_Released)
			{
				speed = 5.f;
				shiftMode = false;
			}
		}
		break;
	case  eKI_LAlt:
		{
			if (event.state == eIS_Pressed)
			{
				altMode = true;
			}
			else if (event.state == eIS_Released)
			{
				altMode = false;
			}
		}
		break;
	case  eKI_LCtrl:
		{
			if (event.state == eIS_Pressed)
			{
				ctrlMode = true;
			}
			else if (event.state == eIS_Released)
			{
				ctrlMode = false;
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
	case eKI_Right:
		{
			if (event.state == eIS_Pressed)
			{
				m_curr_ent++;
				m_curr_ent %= m_ents.size();
				UpdateShader();
			}
		}
		break;
	case eKI_Left:
		{
			if (event.state == eIS_Pressed)
			{
				m_curr_ent--;
				if (m_curr_ent < 0)
				{
					m_curr_ent = m_ents.size() - 1;
				}
				m_curr_ent %= m_ents.size();
				UpdateShader();
			}
		}
		break;
	case eKI_MouseWheelDown:
		{
			if (event.state == eIS_Down)
			{
				m_camdist *= 1.1f;

				updateCam();
			}
		}
		break;
	case eKI_MouseWheelUp:
		{
			if (event.state == eIS_Down)
			{
				m_camdist *= 0.9f;

				updateCam();
			}
		}
		break;
	case eKI_MouseX:
		{
			if (event.state == eIS_Changed && rotateMode)
			{
				m_camera->Rotate( 0, event.value * -0.002f );

				updateCam();

			}
		}
		break;
	case eKI_MouseY:
		{
			if (event.state == eIS_Changed && rotateMode)
			{
				m_camera->Rotate( -event.value * 0.002f, 0 );

				updateCam();
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
	}
	return false;
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
	for (size_t i = 0; i < m_ents.size(); ++i)
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
