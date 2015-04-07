#include "StdAfx.h"
#include "SrEntity.h"

#include "SrMesh.h"
#include "SrMaterial.h"

#include "mmgr/mmgr.h"




SrEntity::SrEntity(void)
{
	m_pos = float3(0.f);
	m_rot = Quat::CreateIdentity();
	m_worldMatrix = float44::CreateIdentity();
	m_mesh = NULL;
	m_visible = true;
	m_scale = float3(1.f);
}

SrEntity::~SrEntity(void)
{

}

float44 SrEntity::getWorldMatrix()
{
	if (m_dirty)
	{
		m_dirty = false;
		Update();
	}
	return m_worldMatrix;
}

void SrEntity::SetPos( float3& pos )
{
	m_pos = pos;
	m_dirty = true;
}

void SrEntity::SetRotation( Quat& rot )
{
	m_rot = rot;
	m_dirty = true;
}

void SrEntity::MoveLocal( float3& trans )
{
	m_pos += (m_rot * trans);

	m_dirty = true;
}

void SrEntity::RotateLocal( float3& angle )
{
	Quat rotX = Quat::CreateRotationX(angle.x);
	Quat rotY = Quat::CreateRotationY(angle.y);
	//Quat rotZ = Quat::CreateRotationZ(angle.z);

	m_rot = m_rot * rotX;
	m_rot = m_rot * rotY;
	//m_rot = rotZ * m_rot;

	m_rot.Normalize();

	m_dirty = true;
}

void SrEntity::Update()
{
	m_worldMatrix = float44::CreateIdentity();
	m_worldMatrix.Set( m_scale, m_rot, m_pos );
}

void SrEntity::Draw()
{
	if (m_mesh && m_visible)
	{
		// draw call
		m_mesh->Draw();
	}
}

void SrEntity::CreateMesh( const char* filename )
{
	m_mesh = gEnv->resourceMgr->LoadMesh(filename);
}

void SrEntity::LoadMaterial( const char* filename )
{
	// hacking load internal
	gEnv->resourceMgr->LoadMaterialLib(filename);
}

SrMaterial* SrEntity::getMaterial( uint32 index )
{
	SrMaterial* ret = NULL;
	if (index < m_mesh->m_primitives.size())
	{
		return m_mesh->m_primitives[index].material;
	}
}

uint32 SrEntity::getMaterialCount()
{
	uint32 ret = 0;
	if (m_mesh)
	{
		ret = m_mesh->m_primitives.size();
	}
	return ret;
}

void SrEntity::SetScale( float3& scale )
{
	m_scale = scale;

	m_dirty = true;
}
