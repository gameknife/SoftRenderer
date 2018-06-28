/**
  @file SrRasterizer.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrRasterizer_h__
#define SrRasterizer_h__

#include "prerequisite.h"
#include "SrShader.h"

class SrRasTaskDispatcher;
class SrSoftRenderer;

inline void FastRasterize( SrRendVertex* out, SrRendVertex* a, SrRendVertex* b, float ratio, float inv_ratio )
{
	__m256 channel0_1_a = _mm256_loadu_ps(&(a->pos.x));
	__m256 channel0_1_b = _mm256_loadu_ps(&(b->pos.x));

	__m256 channel2_3_a = _mm256_loadu_ps(&(a->channel2.x));
	__m256 channel2_3_b = _mm256_loadu_ps(&(b->channel2.x));

	channel0_1_a = _mm256_add_ps( _mm256_mul_ps( channel0_1_a, _mm256_set1_ps(inv_ratio) ), _mm256_mul_ps( channel0_1_b, _mm256_set1_ps(ratio) ));
	channel2_3_a = _mm256_add_ps( _mm256_mul_ps( channel2_3_a, _mm256_set1_ps(inv_ratio) ), _mm256_mul_ps( channel2_3_b, _mm256_set1_ps(ratio) ));

	_mm256_storeu_ps(&(out->pos.x), channel0_1_a);
	_mm256_storeu_ps(&(out->channel2.x), channel2_3_a);
}

inline void FastFinalRasterize( SrRendVertex* out, float w )
{
	__m256 channel0_1_a = _mm256_loadu_ps((float*)(&(out->pos)));
	__m256 channel0_1_b = _mm256_loadu_ps((float*)(&(out->channel2)));

	channel0_1_a = _mm256_div_ps(channel0_1_a, _mm256_set_ps(1,1,1,1,w,w,w,w));
	channel0_1_b = _mm256_div_ps(channel0_1_b, _mm256_set1_ps(w));

	_mm256_storeu_ps((float*)(&(out->pos)), channel0_1_a);
	_mm256_storeu_ps((float*)(&(out->channel2)), channel0_1_b);
}

inline void FixedRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false )
{
	const SrRendVertex* verA = static_cast<const SrRendVertex*>(rInRef0);
	const SrRendVertex* verB = static_cast<const SrRendVertex*>(rInRef1);
	SrRendVertex* verO = static_cast<SrRendVertex*>(rOut);

	// 线性插值project space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// 已经除w
	// 直接插值，其他channel
	verO->channel1 = SrFastLerp( verA->channel1, verB->channel1, ratio, inv_ratio );
	verO->channel2 = SrFastLerp( verA->channel2, verB->channel2, ratio, inv_ratio );
	verO->channel3 = SrFastLerp( verA->channel3, verB->channel3, ratio, inv_ratio );

	// 对于scanline扫描的，将透视插值坐标，插值回正常值
	if (final)
	{
		verO->channel1 /= verO->pos.w;
		verO->channel2 /= verO->pos.w;
		verO->channel3 /= verO->pos.w;
	}
};

/**
 *@brief 渲染primitive
 */
SR_ALIGN struct SrRendPrimitve
{
	SrVertexBuffer*		vb;					///< vertex buffer
	SrIndexBuffer*		ib;
	SrShaderContext		shaderConstants;
	const SrSwShader*			shader;

	void * operator new(size_t size);
	void operator delete(void *memoryToBeDeallocated);
};

/**
 *@brief 光栅化处理器
 */
SR_ALIGN class SrRasterizer
{
public:
	/**
	 *@brief 光栅化用三角形，引用渲染顶点
	 */
	SR_ALIGN struct SrRastTriangle
	{
		SrRendVertex p[3];
		SrRendPrimitve* primitive;
	};


public:
	SrRasterizer(void);
	virtual ~SrRasterizer(void);

	/**
	 *@brief 初始化光栅化处理器
	 */
	void Init(SrSoftRenderer* renderer);

	// 接受外界来自primitive的渲染调用，缓存
	bool DrawPrimitive( SrPrimitve* primitive );
	bool DrawLine(const float3& from, const float3& to);
	bool DrawRHZPrimitive( SrRendPrimitve& rendPrimitive );

	// 帧末尾，对缓存的primitive进行渲染
	void Flush();

	// 光栅化primitive入口
	static void ProcessRasterizer( SrRendPrimitve* in_primitive, SrFragment* out_gBuffer );

	// 对三角形进行针对视锥的裁剪，并提交
	static void RasterizeTriangle_Clip( SrRastTriangle& tri, float zNear, float zFar );
	// 光栅化处理后的三角形入口
	static void RasterizeTriangle( SrRastTriangle& tri, bool subtri = false );
	
	//////////////////////////////////////////////////////////////////////////
	// 内部光栅化函数

	// 光栅化平顶三角形
	static void Rasterize_Top_Tri_F( SrRastTriangle& tri );
	// 光栅化平底三角形
	static void Rasterize_Bottom_Tri_F( SrRastTriangle& tri );
	// 光栅化扫描线
	static void Rasterize_ScanLine( uint32 line, float fstart, float fcount, const void* vertA, const void* vertB, SrRendPrimitve* primitive, ERasterizeMode rMode = eRm_WireFrame, bool toptri = true );
	// 光栅化 需要裁剪的 扫描线
	static void Rasterize_ScanLine_Clipped( uint32 line, float fstart, float fcount, float clipStart, float clipCount, const void* vertA, const void* vertB, SrRendPrimitve* primitive, ERasterizeMode rMode = eRm_Solid );
	// 光栅化并写pixel
	static void Rasterize_WritePixel( const void* vertA, const void* vertB, float ratio, SrFragment* thisBuffer, SrRendPrimitve* primitive, uint32 address );

	static int Draw_Line(int x0, int y0, int x1, int y1, int color, uint32 *vb_start, int lpitch);
	static int Clip_Line(int &x1,int &y1,int &x2, int &y2);
	static int Draw_Clip_Line(int x0,int y0, int x1, int y1, int color, uint32 *dest_buffer, int lpitch);
public:
	std::list<SrRendPrimitve*> m_rendPrimitives;			///< 渲染Primitive队列，每帧接受DrawPrimitive调用后缓存在这里，flush结束时删除

	std::list<SrRendPrimitve*> m_rendPrimitivesRHZ;		///< 渲染Primitive队列，每帧接受DrawPrimitive调用后缓存在这里，flush结束时删除

	std::vector<float4> m_rendDynamicVertex;

	SrTexture* m_MemSBuffer;								///< 动态纹理，当前帧
	SrTexture* m_BackS1Buffer;								///< 动态纹理，用于缓存jitAA开启时的上一帧
	SrTexture* m_BackS2Buffer;								///< 动态纹理，用于缓存jitAA开启时的上一帧

	SrRasTaskDispatcher* m_rasTaskDispatcher;				///< Task分发器，用于任务动态调度
	class SrSoftRenderer* m_renderer;
};

#endif // SrRasterizer_h__


