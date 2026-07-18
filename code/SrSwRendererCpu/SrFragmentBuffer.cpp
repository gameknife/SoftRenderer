#include "StdAfx.h"
#include "SrFragmentBuffer.h"
#include "SrSoftRenderer.h"

SrFragmentBuffer::SrFragmentBuffer( int width, int height, SrSoftRenderer* renderer)
	: m_width(width), m_height(height), m_tileReady(NULL), m_tileSize(0), m_tileColumns(0), m_renderer(renderer)
{
	uint32 size = width * height;
	//fragBuffer = new SrFragmentBuffer[size];
	fBuffer = (SrFragment*)(_mm_malloc_custom( sizeof(SrFragment) * size, 16 ));

	memset(fBuffer, 0, size * sizeof(SrFragment));

	//fragBufferPitch = sizeof(SrFragment) * width;
	m_fBufferIndices = gEnv->resourceMgr->AllocateIndexBuffer(width * height);

	zBuffer = new float[size];
	memset(zBuffer, 0, size * sizeof(float));
	#ifdef RASTERIZER_SYNC
	fragBufferSync = new SrFragmentBufferSync[size];
	#endif
}

void SrFragmentBuffer::ConfigureRelaxedTileReads(std::atomic<LONG>* tileReady, uint32 tileSize, uint32 tileColumns)
{
	m_tileReady = tileReady;
	m_tileSize = tileSize;
	m_tileColumns = tileColumns;
}

void SrFragmentBuffer::DisableRelaxedTileReads()
{
	m_tileReady = NULL;
	m_tileSize = 0;
	m_tileColumns = 0;
}

SrFragmentBuffer::~SrFragmentBuffer(void)
{
	_mm_free_custom(fBuffer);
	gEnv->resourceMgr->DeleteIndexBuffer(m_fBufferIndices);
	#ifdef RASTERIZER_SYNC
	delete[] fragBufferSync;
	#endif
	delete[] zBuffer;
}

float3 SrFragmentBuffer::GetNormal( float2& texcoord ) const
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
