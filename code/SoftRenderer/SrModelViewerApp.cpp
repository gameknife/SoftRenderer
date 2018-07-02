#include "StdAfx.h"
#include "SrModelViewerApp.h"
#include "SrScene.h"
#include "SrEntity.h"
#include "SrMaterial.h"
#include "SrCamera.h"

#include <tchar.h>
#include <io.h>

inline bool is_end_with_slash( const TCHAR* filename )
{
	size_t len = _tcslen(filename);
	if (filename[len-1] == _T('\\') || filename[len-1] == _T('/'))
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline void enum_all_files_in_folder( const TCHAR* root_path,std::vector<std::string>& result,bool inc_sub_folders/*=false*/ )
{

	if(!root_path)
	{
		return;
	}

	//要查找的目录
	std::string str = root_path;
	if (!is_end_with_slash(root_path))
	{
		str += _T("/");
	}
	std::stack<std::string> path_buf;
	path_buf.push(str);

	while(path_buf.size())
	{
		//取出来栈顶item
		std::string path = path_buf.top();
		path_buf.pop();
		size_t k=path_buf.size();

		std::string find_path = path + _T("*.*");

		_tfinddata_t file;
		intptr_t longf = _tfindfirst(find_path.c_str(), &file);

		if(longf !=-1)
		{
			std::string tempName;
			while(_tfindnext(longf, &file ) != -1)
			{
				tempName = _T("");
				tempName = file.name;
				if (tempName == _T("..") || tempName == _T("."))
				{
					continue;
				}
				if (file.attrib == _A_SUBDIR)
				{
					if (inc_sub_folders)
					{
						tempName += _T("\\");
						tempName = path + tempName;
						path_buf.push(tempName);
					}
				}
				else
				{
					result.push_back(tempName);
				}
			}
		}
		_findclose(longf);
	}
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
	// 打开渲染特性
	g_context->OpenFeature(eRFeature_MThreadRendering);
	g_context->OpenFeature(eRFeature_JitAA);
	g_context->OpenFeature(eRFeature_LinearFiltering);

	// 创建场景
	m_scene = new SrScene;
	gEnv->sceneMgr = m_scene;

	m_curr_ent = 0;
	m_ent = m_scene->CreateEntity("object", "\\media\\modelviewer\\teapot.obj", "\\media\\modelviewer\\teapot.mtl");
	m_ents.push_back(m_ent);
	m_ent->SetScale(float3(40, 40, 40));
	
	//SwitchSSAO();

	// 创建相机
	m_camera = m_scene->CreateCamera("cam0");
	m_camera->setPos(float3(0,0,-15));
	m_camera->setFov(68.0f);
	m_camera->Rotate( 0.1f, 0.3f );
	m_scene->PushCamera(m_camera);
	m_camdist = 15.0f;
	updateCam();

	// 添加一个主光
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
	sprintf_s( buffer, "Switch Shade Mode: %d", m_shade_mode );
	gEnv->renderer->DrawScreenText( buffer, startxL, starty, 1, SR_UICOLOR_NORMAL );

	gEnv->renderer->DrawScreenText( "[Press K]", keyL, starty += 10, 1, SR_UICOLOR_MAIN);
	sprintf_s( buffer, "DotCoverage: %s", g_context->IsFeatureEnable(eRFeature_DotCoverageRendering) ? "on" : "off" );
	gEnv->renderer->DrawScreenText( buffer, startxL, starty, 1, SR_UICOLOR_NORMAL );

	gEnv->renderer->DrawScreenText( "[CamCtrl]", keyL, starty += 10, 1, SR_UICOLOR_MAIN);
	sprintf_s( buffer, "WASD Move Cam | Mouse L+Drag Rotate Cam" );
	gEnv->renderer->DrawScreenText( buffer, startxL, starty, 1, SR_UICOLOR_NORMAL );
}

void SrModelViewerApp::OnDestroy()
{
	// 删除场景
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