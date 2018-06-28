#include "stdafx.h"
#include "SrRasTasks.h"
#include "SrMaterial.h"
#include "SrProfiler.h"
#include "SrRasterizer.h"
#include "SrSwShader.h"

#include "mmgr.h"

SrRasTask_Pixel::SrRasTask_Pixel( int indexStart, int indexEnd, uint32* indexBuffer, SrFragment* gBuffer, uint32* oBuffer ):
	m_indexStart(indexStart),
	m_indexEnd(indexEnd),
	m_indexBuffer(indexBuffer),
	m_gBuffer(gBuffer),
	m_oBuffer(oBuffer)
{

}

void SrRasTask_Pixel::Execute()
{
	for ( uint32 i = m_indexStart; i < m_indexEnd; ++i)
	{
		assert(  m_indexBuffer[i] >=0 &&  m_indexBuffer[i] < g_context->width * g_context->height );
		int index = m_indexBuffer[i];
		SrFragment* in = m_gBuffer + index;
		uint32* out = m_oBuffer + index;
		assert( in->primitive );
		//assert( in->primitive->material );
		assert( in->primitive->shader );

		in->primitive->shader->ProcessPixel( out, in, &(in->primitive->shaderConstants), m_indexBuffer[i] );
	}
}

SrRasTask_Vertex::SrRasTask_Vertex( int indexStart, int indexEnd, SrVertexBuffer* vb, SrRendPrimitve* primitive ):
	m_indexStart(indexStart),
	m_indexEnd(indexEnd),
	m_vb(vb),
	m_primitive(primitive)
{

}

void SrRasTask_Vertex::Execute()
{
	for ( uint32 i = m_indexStart; i < m_indexEnd ; ++i)
	{
		assert(  i >=0 &&  i < m_vb->elementCount );
		// 取得顶点指针
		void* vsOut = (void*)(m_vb->data + i * m_vb->elementSize);
		SrRendVertex tmp = *((SrRendVertex*)vsOut);
		// 处理
		m_primitive->shader->ProcessVertex( vsOut, 0, 0, &tmp, 0, 0, &(m_primitive->shaderConstants) );
	}
}

SrRasTask_JitAA::SrRasTask_JitAA( int indexStart, int indexEnd, uint32* inBufferA, uint32* inBufferB, uint32* oBuffer ):
	m_indexStart(indexStart),
	m_indexEnd(indexEnd),
	m_inBufferA(inBufferA),
	m_inBufferB(inBufferB),
	m_oBuffer(oBuffer)
{

}

void SrRasTask_JitAA::Execute()
{
	for (uint32 i = m_indexStart ; i < m_indexEnd; ++i)
	{
		assert(  i >=0 &&  i < g_context->width * g_context->height );
		if (g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
		{
			*(m_oBuffer + i) = SrColorMerge( *(m_inBufferA + i ), *(m_inBufferB + i ), false);
		}
		else
		{
			*(m_oBuffer + i) = SrColorMerge( *(m_inBufferA + i ), *(m_inBufferB + i ), true);
		}
		
	}
}

SrRasTask_Clear::SrRasTask_Clear( void* dst, int size, uint8 val ):
	m_dst(dst),
	m_size(size),
	m_val(val)
{

}

void SrRasTask_Clear::Execute()
{
	memset( m_dst, m_val, m_size );
}

SrRasTask_Rasterize::SrRasTask_Rasterize( SrRendPrimitve* pri, SrVertexBuffer* vb, SrIndexBuffer* ib, uint32 indexStart, uint32 indexEnd ):
	m_primitive(pri),
	m_vb(vb),
	m_ib(ib),
	m_indexStart(indexStart),
	m_indexEnd(indexEnd)
{

}

void SrRasTask_Rasterize::Execute()
{
	// 用ib索引，构造三角形进行光栅化
	if (m_vb && m_ib)
	{
		for ( uint32 i=m_indexStart; i < m_indexEnd; ++i )
		{
			SrRasterizer::SrRastTriangle tri;
			tri.p[0] = *(SrRendVertex*)(m_vb->data + m_ib->data[i * 3 + 0] * m_vb->elementSize);
			tri.p[1] = *(SrRendVertex*)(m_vb->data + m_ib->data[i * 3 + 1] * m_vb->elementSize);
			tri.p[2] = *(SrRendVertex*)(m_vb->data + m_ib->data[i * 3 + 2] * m_vb->elementSize);
			tri.primitive = m_primitive;

			// 提交三角形进行裁剪
			SrRasterizer::RasterizeTriangle_Clip(tri, g_context->viewport.n, g_context->viewport.f);
		}
	}
	else if (m_vb)
	{
		// without ib
	}
}
