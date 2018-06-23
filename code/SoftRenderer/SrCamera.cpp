#include "StdAfx.h"
#include "SrCamera.h"

#include "mmgr.h"
SrCamera::SrCamera( const float3& p, const Quat& r, float fov, float zN, float zF ) :m_pos(p)
	,m_rot(r)
	,m_fov(fov)
	,m_zNear(zN)
	,m_zFar(zF)
	,dirt(false)
{
	Update();
}

SrCamera::~SrCamera(void)
{
}

void SrCamera::Update()
{
	m_rot.Normalize();

	cachedView.LookatLH( m_pos, m_pos + m_rot.GetRow2(), float3(0,1,0) );
	float aspect = (float)(g_context->width) / ((float)g_context->height);
	cachedProj.PerspectiveFovLH( m_fov / 180.f * SR_PI, aspect, m_zNear, m_zFar );

	// if jit aa on [11/7/2012 Kaiming]
	if (g_context->IsFeatureEnable(eRFeature_JitAA))
	{
		int i = 1;
		if (gEnv->renderer->getFrameCount() % 2 ==0)
		{
			i = -1;
		}
		cachedProj.m20 += (0.5f / (float)g_context->width) * i;
		cachedProj.m21 += (0.5f / (float)g_context->height) * i;
	}

	cachedViewProj = cachedView * cachedProj;

	float3 lookDir = (m_rot.GetRow2());
	lookDir.normalize();
	float3 m_farCenter = lookDir * m_zFar;
	float3 rightDir = float3(0,1,0) % lookDir;
	rightDir.normalize();
	float3 headDir = lookDir % rightDir;
	headDir.normalize();

	float h = tanf( m_fov / 180.f * SR_PI * 0.5f ) * m_zFar;
	float w = h * aspect;

	rightDir *= w;
	headDir *=  h;

	m_farClipVertex[0] = m_farCenter - rightDir + headDir;
	m_farClipVertex[1] = m_farCenter - rightDir - headDir;
	m_farClipVertex[2] = m_farCenter + rightDir + headDir;
	m_farClipVertex[3] = m_farCenter + rightDir - headDir;

// 	gEnv->renderer->DrawLine( m_pos, m_pos + m_farClipVertex[0] );
// 	gEnv->renderer->DrawLine( m_pos, m_pos + m_farClipVertex[1] );
// 	gEnv->renderer->DrawLine( m_pos, m_pos + m_farClipVertex[2] );
// 	gEnv->renderer->DrawLine( m_pos, m_pos + m_farClipVertex[3] );
// 	gEnv->renderer->DrawLine( m_pos + m_farClipVertex[0], m_pos + m_farClipVertex[3] );
// 	gEnv->renderer->DrawLine( m_pos + m_farClipVertex[1], m_pos + m_farClipVertex[2] );
// 	gEnv->renderer->DrawLine( m_pos + m_farClipVertex[0], m_pos + m_farClipVertex[1] );
// 	gEnv->renderer->DrawLine( m_pos + m_farClipVertex[2], m_pos + m_farClipVertex[3] );
}

void SrCamera::Move( const float3& localTranslate )
{
	
	m_pos += (localTranslate * m_rot);

	dirt = true;
}

void SrCamera::Rotate( float yaw, float roll )
{
	Quat rollQ = Quat::CreateRotationY( roll );
	Quat yawQ = Quat::CreateRotationX( yaw );

	m_rot = m_rot * rollQ;
	m_rot = yawQ * m_rot;

	dirt = true;
}

float44 SrCamera::getViewProjMatrix()
{
	if (dirt)
	{
		dirt = false;
		Update();
	}
	return cachedViewProj;
}

float44 SrCamera::getViewMatrix()
{
	if (dirt)
	{
		dirt = false;
		Update();
	}
	return cachedView;
}

void SrCamera::setPos( const float3& pos )
{
	m_pos = pos;

	dirt = true;
}

void SrCamera::setRot( const Quat& rot )
{
	m_rot = rot;

	dirt = true;
}

void SrCamera::setFov( float fov )
{
	m_fov = fov;

	dirt = true;
}

void SrCamera::MarkDirty()
{
	dirt = true;
}
