#include "StdAfx.h"
#include "SrScene.h"
#include "SrEntity.h"
#include "SrCamera.h"

#include "mmgr.h"


SrScene::SrScene(void)
{
	// ��ʼ�����
	m_skyLightColor = SR_ARGB_F(255, 25, 43, 89);
}


SrScene::~SrScene(void)
{
	// release Entitys
	SrEntityLibrary::iterator it = m_entityLib.begin();
	for (; it != m_entityLib.end(); ++it)
	{
		if( it->second )
		{
			delete it->second;
		}
	}

	// release Entitys
	SrCameraLibrary::iterator itcam = m_cameraLib.begin();
	for (; itcam != m_cameraLib.end(); ++itcam)
	{
		if( itcam->second )
		{
			delete itcam->second;
		}
	}

	ClearLight();
}

SrEntity* SrScene::CreateEntity( const char* name, const char* meshFilename, const char* matFilename, float3& pos /*= float3(0,0,0)*/, Quat& rot /*= Quat::CreateIdentity()*/, SrEntity* pent  )
{
	SrEntityLibrary::iterator it = m_entityLib.find(name);

	if (it != m_entityLib.end())
	{
		return it->second;
	}

	// not find, create and pushinto
	SrEntity* ent = NULL;
	if (!pent)
	{
		ent = new SrEntity;
	}
	else
	{
		ent = pent;
	}
	
	ent->LoadMaterial(matFilename);
	ent->CreateMesh(meshFilename);
	
	ent->SetPos(pos);
	ent->SetRotation(rot);

	m_entityLib.insert(SrEntityLibrary::value_type( name, ent ));

	return ent;
}

SrEntity* SrScene::GetEntity( const char* name )
{
	SrEntityLibrary::iterator it = m_entityLib.find(name);

	if (it != m_entityLib.end())
	{
		return it->second;
	}

	return NULL;
}

void SrScene::RemoveEntity( SrEntity* target )
{
	SrEntityLibrary::iterator it = m_entityLib.begin();
	for (; it != m_entityLib.end(); ++it)
	{
		if (it->second == target)
		{
			m_entityLib.erase(it);
			break;
		}
	}
}

void SrScene::RemoveEntity( const char* name )
{
	SrEntityLibrary::iterator it = m_entityLib.find(name);

	if (it != m_entityLib.end())
	{
		m_entityLib.erase(it);
	}
}

SrCamera* SrScene::CreateCamera( const char* name )
{
	SrCameraLibrary::iterator it = m_cameraLib.find(name);

	if (it != m_cameraLib.end())
	{
		return it->second;
	}

	SrCamera* cam = new SrCamera(float3(0,0,0), Quat::CreateIdentity(), 60.f, 1.0f, 8000.f);

	m_cameraLib.insert(SrCameraLibrary::value_type( name, cam ));

	return cam;
}

SrCamera* SrScene::GetCamera( const char* name )
{
	SrCameraLibrary::iterator it = m_cameraLib.find(name);

	if (it != m_cameraLib.end())
	{
		return it->second;
	}

	return NULL;
}

void SrScene::PushCamera( SrCamera* cam )
{
	m_cameraStack.push(cam);
}

void SrScene::PopCamera()
{
	m_cameraStack.pop();
}

void SrScene::Update()
{
	
	if (m_cameraStack.empty())
	{
		assert(0);
		return;
	}

	SrCamera* cam = m_cameraStack.top();

	SrEntityLibrary::iterator it = m_entityLib.begin();
	for (; it != m_entityLib.end(); ++it)
	{
		if (it->second)
		{
			it->second->Update(gEnv->timer->getElapsedTime());
		}
	}

	RenderVisbility(cam);
}

void SrScene::RenderVisbility( SrCamera* cam )
{
	// set cam to render
	g_context->viewport.n = cam->getZnear();
	g_context->viewport.f = cam->getZfar();

	// not cull now

	cam->MarkDirty();


	SrEntityLibrary::iterator it = m_entityLib.begin();
	for (; it != m_entityLib.end(); ++it)
	{
		if (it->second)
		{
			gEnv->renderer->SetMatrix(eMd_WorldViewProj, it->second->getWorldMatrix() * cam->getViewProjMatrix() );
			gEnv->renderer->SetMatrix(eMd_World, it->second->getWorldMatrix() );
			gEnv->renderer->SetMatrix(eMd_ViewInverse, cam->getViewMatrix().GetInverse());
			it->second->Draw();
		}
	}
}


SrLight* SrScene::AddLight()
{
	SrLight* ret = new SrLight;
	m_lightList.push_back(ret);

	return ret;
}

void SrScene::RemoveLight( SrLight* tgt )
{
	SrLightList::iterator it = m_lightList.begin();
	for (; it != m_lightList.end(); ++it)
	{
		if ( *it == tgt)
		{
			delete (*it);
			m_lightList.erase(it);
			break;
		}
	}
}

void SrScene::ClearLight()
{
	SrLightList::iterator it = m_lightList.begin();
	for (; it != m_lightList.end(); ++it)
	{
		delete (*it);
	}

	m_lightList.clear();
}
