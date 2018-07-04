#include "StdAfx.h"
#include "SrFragmentBuffer.h"
#include "SrSoftRenderer.h"

//std::map<const void*, void*> m_align_pt_mapper;

SrFragmentBuffer::SrFragmentBuffer( int width, int height, SrSoftRenderer* renderer):m_width(width), m_height(height), m_renderer(renderer)
{
	uint32 size = width * height;
	//fragBuffer = new SrFragmentBuffer[size];
	fBuffer = (SrFragment*)(_mm_malloc_custom( sizeof(SrFragment) * size, 16 ));

	memset(fBuffer, 0, size * sizeof(SrFragment));

	//fragBufferPitch = sizeof(SrFragment) * width;
	m_fBufferIndices = gEnv->resourceMgr->AllocateIndexBuffer(width * height);

	fragBufferSync = new SrFragmentBufferSync[size];

	zBuffer = new float[size];
}


SrFragmentBuffer::~SrFragmentBuffer(void)
{
	_mm_free_custom(fBuffer);
	gEnv->resourceMgr->DeleteIndexBuffer(m_fBufferIndices);
	delete[] fragBufferSync;
	delete[] zBuffer;
}

float3 SrFragmentBuffer::GetNormal( const float2& texcoord ) const
{
	// ��������warp
	float u = texcoord.x - floor(texcoord.x);
	float v = texcoord.y - floor(texcoord.y);

	// �ٽ������
	u *= (m_width);
	v *= (m_height);

	// get int
	int x = (int)( u );
	int y = (int)( v );
	x = x % m_width;
	y = y % m_height;

	return fBuffer[y * m_width + x].normal_ty.xyz;
}

void SrFragmentBuffer::Clear()
{
	memset( zBuffer, 0, m_width * m_height * sizeof(float));
}
