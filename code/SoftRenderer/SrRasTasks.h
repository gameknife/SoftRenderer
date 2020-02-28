/**
  @file SrRasTasks.h
  
  @author Kaiming

  ver:1.0
   
 */

#ifndef SrRasTasks_h__
#define SrRasTasks_h__

#include "prerequisite.h"
#include "SrRasTaskDispatcher.h"

struct SrRasTask_Pixel : public SrRasTask
{
	SrRasTask_Pixel() {}
	SrRasTask_Pixel(int indexStart, int indexEnd, uint32* indexBuffer, SrFragment* gBuffer, uint32* oBuffer);
	~SrRasTask_Pixel() {}
	virtual void Execute();

	uint32 m_indexStart;
	uint32 m_indexEnd;
	uint32* m_indexBuffer;

	SrFragment* m_gBuffer;
	uint32*	m_oBuffer;
};

struct SrRasTask_Vertex : public SrRasTask
{
	SrRasTask_Vertex(int indexStart, int indexEnd, SrVertexBuffer* vb, SrRendPrimitve* primitive );
	~SrRasTask_Vertex() {}

	virtual void Execute();

	uint32 m_indexStart;
	uint32 m_indexEnd;
	SrVertexBuffer* m_vb;
	SrRendPrimitve* m_primitive;
};

struct SrRasTask_JitAA : public SrRasTask
{
	SrRasTask_JitAA(int indexStart, int indexEnd, uint32* inBufferA, uint32* inBufferB, uint32* oBuffer);
	~SrRasTask_JitAA() {}
	virtual void Execute();

	uint32 m_indexStart;
	uint32 m_indexEnd;
	uint32* m_inBufferA;
	uint32* m_inBufferB;
	uint32*	m_oBuffer;
};

struct SrRasTask_Clear : public SrRasTask
{
	SrRasTask_Clear(void* dst, int size, uint8 val = 0);
	~SrRasTask_Clear() {}
	virtual void Execute();

	void* m_dst;
	int m_size;
	uint8 m_val;
};

struct SrRasTask_Rasterize : public SrRasTask
{
	SrRasTask_Rasterize(SrRendPrimitve* primitive, SrVertexBuffer* vb, SrIndexBuffer* ib, uint32 indexStart, uint32 indexEnd);
	~SrRasTask_Rasterize() {}
	virtual void Execute();

	SrRendPrimitve* m_primitive;
	SrVertexBuffer* m_vb;
	SrIndexBuffer* m_ib;
	uint32 m_indexStart;
	uint32 m_indexEnd;
};
#endif