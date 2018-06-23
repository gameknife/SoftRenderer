/**
  @file SrRasTasks.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrRasTasks_h__
#define SrRasTasks_h__

#include "prerequisite.h"
#include "SrRasTaskDispatcher.h"

/**
 @brief 像素处理任务
 @remark 处理fragBuffer中的一个block，执行pixelshader。
 */
struct SrRasTask_Pixel : public SrRasTask
{
	SrRasTask_Pixel(int indexStart, int indexEnd, uint32* indexBuffer, SrFragment* gBuffer, uint32* oBuffer);
	~SrRasTask_Pixel() {}
	virtual void Execute();

	uint32 m_indexStart;
	uint32 m_indexEnd;
	uint32* m_indexBuffer;

	SrFragment* m_gBuffer;
	uint32*	m_oBuffer;
};

/**
 @brief 顶点处理任务
 @remark 处理vertexBuffer中的一个block，执行vertexshader。
 */
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

/**
 @brief 抖动抗锯齿处理任务
 @remark 处理backBuffer和backBuffer1中的一个block，混合颜色后输出到硬件outBuffer中。
 */
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

/**
 @brief 内存clear任务
 @remark 清理目标内存
 */
struct SrRasTask_Clear : public SrRasTask
{
	SrRasTask_Clear(void* dst, int size, uint8 val = 0);
	~SrRasTask_Clear() {}
	virtual void Execute();

	void* m_dst;
	uint8 m_val;
	int m_size;
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