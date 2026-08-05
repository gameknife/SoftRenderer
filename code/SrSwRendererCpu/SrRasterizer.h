/**
  @file SrRasterizer.h
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrRasterizer_h__
#define SrRasterizer_h__

#include "prerequisite.h"
#include "SrShader.h"
#include "SrTileTypes.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class SrRasTaskDispatcher;
class SrSoftRenderer;

inline void FastRasterize( SrRendVertex* out, SrRendVertex* a, SrRendVertex* b, float ratio, float inv_ratio )
{
#if defined(__AVX__)
	__m256 channel0_1_a = _mm256_loadu_ps(&(a->pos.x));
	__m256 channel0_1_b = _mm256_loadu_ps(&(b->pos.x));

	__m256 channel2_3_a = _mm256_loadu_ps(&(a->channel2.x));
	__m256 channel2_3_b = _mm256_loadu_ps(&(b->channel2.x));

	channel0_1_a = _mm256_add_ps( _mm256_mul_ps( channel0_1_a, _mm256_set1_ps(inv_ratio) ), _mm256_mul_ps( channel0_1_b, _mm256_set1_ps(ratio) ));
	channel2_3_a = _mm256_add_ps( _mm256_mul_ps( channel2_3_a, _mm256_set1_ps(inv_ratio) ), _mm256_mul_ps( channel2_3_b, _mm256_set1_ps(ratio) ));

	_mm256_storeu_ps(&(out->pos.x), channel0_1_a);
	_mm256_storeu_ps(&(out->channel2.x), channel2_3_a);
#else
	float* destination = &out->pos.x;
	const float* left = &a->pos.x;
	const float* right = &b->pos.x;
	for (int i = 0; i < 16; ++i) destination[i] = left[i] * inv_ratio + right[i] * ratio;
#endif
}

inline void FastFinalRasterize( SrRendVertex* out, float w )
{
#if defined(__AVX__)
	__m256 channel0_1_a = _mm256_loadu_ps((float*)(&(out->pos)));
	__m256 channel0_1_b = _mm256_loadu_ps((float*)(&(out->channel2)));

	channel0_1_a = _mm256_div_ps(channel0_1_a, _mm256_set_ps(1,1,1,1,w,w,w,w));
	channel0_1_b = _mm256_div_ps(channel0_1_b, _mm256_set1_ps(w));

	_mm256_storeu_ps((float*)(&(out->pos)), channel0_1_a);
	_mm256_storeu_ps((float*)(&(out->channel2)), channel0_1_b);
#else
	out->pos /= w;
	out->channel2 /= w;
	out->channel3 /= w;
#endif
}

inline void FixedRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false )
{
	const SrRendVertex* verA = static_cast<const SrRendVertex*>(rInRef0);
	const SrRendVertex* verB = static_cast<const SrRendVertex*>(rInRef1);
	SrRendVertex* verO = static_cast<SrRendVertex*>(rOut);

	// ���Բ�ֵproject space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// �Ѿ���w
	// ֱ�Ӳ�ֵ������channel
	verO->channel1 = SrFastLerp( verA->channel1, verB->channel1, ratio, inv_ratio );
	verO->channel2 = SrFastLerp( verA->channel2, verB->channel2, ratio, inv_ratio );
	verO->channel3 = SrFastLerp( verA->channel3, verB->channel3, ratio, inv_ratio );

	// ����scanlineɨ��ģ���͸�Ӳ�ֵ���꣬��ֵ������ֵ
	if (final)
	{
		verO->channel1 /= verO->pos.w;
		verO->channel2 /= verO->pos.w;
		verO->channel3 /= verO->pos.w;
	}
};

/**
 *@brief ��Ⱦprimitive
 */
SR_ALIGN struct SrRendPrimitve
{
	const SrVertexBuffer*	sourceVb;			///< immutable normalized VS input
	SrVertexBuffer*		vb;					///< frame-local VS output
	SrVertexBuffer		frameVb;			///< frame-local VS output descriptor
	SrIndexBuffer*		ib;
	SrShaderContext		shaderConstants;
	const SrSwShader*			shader;
	uint32				drawSequence;
	uint32				frameNumber;
	float				zNear;
	float				zFar;

	void * operator new(size_t size);
	void operator delete(void *memoryToBeDeallocated);
};

/**
 *@brief ��դ��������
 */
SR_ALIGN class SrRasterizer
{
public:
	struct SrTextureGradientPlane
	{
		float uOverWDx;
		float uOverWDy;
		float vOverWDx;
		float vOverWDy;
		float inverseWDx;
		float inverseWDy;
	};

	/**
	 *@brief ��դ���������Σ�������Ⱦ����
	 */
	SR_ALIGN struct SrRastTriangle
	{
		SrRendVertex p[3];
		SrRendPrimitve* primitive;
		SrTextureGradientPlane textureGradient;
		bool prepared;
		SrRastTriangle()
			: primitive(NULL), textureGradient{}, prepared(false) {}
	};


public:
	void* operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void* memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}

	SrRasterizer(void);
	virtual ~SrRasterizer(void);

	/**
	 *@brief ��ʼ����դ��������
	 */
	void Init(SrSoftRenderer* renderer);

	// �����������primitive����Ⱦ���ã�����
	bool DrawPrimitive( SrPrimitve* primitive );
	bool DrawLine(const float3& from, const float3& to);
	bool DrawRHZPrimitive( SrRendPrimitve& rendPrimitive );

	// ֡ĩβ���Ի����primitive������Ⱦ
	void Flush();
	void FlushTileBinning();
	static void WaitForFrameSurface(void* context);
	void SubmitFrameAsync();
	bool WaitForFrameAsync();
	void SignalFrameSurface(bool ready);
	void StopFrameThread();
	void FrameThreadMain();
	bool IsFrameSurfaceReady() const { return m_frameSurfaceReady; }
	void MarkFrameReadyForPresent() { m_frameSurfaceReady = false; m_presentPending = true; }
	void PresentPendingFrame();

	// ��դ��primitive���
	static void ProcessRasterizer( SrRendPrimitve* in_primitive, SrFragment* out_gBuffer );

	// �������ν��������׶�Ĳü�����ύ
	static void RasterizeTriangle_Clip( SrRastTriangle& tri, float zNear, float zFar );
	static void SubmitClippedTriangle( SrRastTriangle& tri );
	static bool PrepareTriangle( SrRastTriangle& tri );
	static void SetTriangleCollector(std::vector<SrRastTriangle>* collector);
	static void SetTileRect(const SrTileRect* rect);
	// ��դ�����������������
	static void RasterizeTriangle( SrRastTriangle& tri, bool subtri = false );
	
	//////////////////////////////////////////////////////////////////////////
	// �ڲ���դ������

	// ��դ��ƽ��������
	static void Rasterize_Top_Tri_F( SrRastTriangle& tri );
	// ��դ��ƽ��������
	static void Rasterize_Bottom_Tri_F( SrRastTriangle& tri );
	// ��դ��ɨ����
	static void Rasterize_ScanLine( uint32 line, float fstart, float fcount, const void* vertA, const void* vertB, SrRendPrimitve* primitive, const SrTextureGradientPlane& textureGradient, ERasterizeMode rMode = eRm_WireFrame, bool toptri = true );
	// ��դ�� ��Ҫ�ü�� ɨ����
	static void Rasterize_ScanLine_Clipped( uint32 line, float fstart, float fcount, float clipStart, float clipCount, const void* vertA, const void* vertB, SrRendPrimitve* primitive, const SrTextureGradientPlane& textureGradient, ERasterizeMode rMode = eRm_Solid );
	// ��դ����дpixel
	static void Rasterize_WritePixel( const void* vertA, const void* vertB, float ratio, SrFragment* thisBuffer, SrRendPrimitve* primitive, uint32 address, const SrTextureGradientPlane& textureGradient );

	static int Draw_Line(int x0, int y0, int x1, int y1, int color, uint32 *vb_start, int lpitch);
	static int Clip_Line(int &x1,int &y1,int &x2, int &y2);
	static int Draw_Clip_Line(int x0,int y0, int x1, int y1, int color, uint32 *dest_buffer, int lpitch);
public:
	std::list<SrRendPrimitve*> m_rendPrimitives;			///< ��ȾPrimitive���У�ÿ֡����DrawPrimitive���ú󻺴������flush����ʱɾ��
	std::list<SrRendPrimitve*> m_buildRendPrimitives;

	std::list<SrRendPrimitve*> m_rendPrimitivesRHZ;		///< ��ȾPrimitive���У�ÿ֡����DrawPrimitive���ú󻺴������flush����ʱɾ��
	std::list<SrRendPrimitve*> m_buildRendPrimitivesRHZ;

	std::vector<float4> m_rendDynamicVertex;
	std::vector<float4> m_buildDynamicVertex;
	SrTileLayout m_tileLayout;
	SrTileBins m_tileBins;
	std::vector<SrRendPrimitve*> m_tileDraws;
	std::vector<std::vector<SrRastTriangle> > m_tileDrawTriangles;
	std::vector<SrRastTriangle> m_tilePreparedTriangles;
	std::vector<SrRastTriangle*> m_tilePreparedTriangleRefs;
	std::vector<SrPreparedTriangleBounds> m_tileTriangleBounds;
	std::vector<uint32> m_tileBinCounts;
	std::vector<uint32> m_tileBinStarts;
	std::vector<uint8> m_tileDependencyFlags;
	std::vector<std::vector<uint32> > m_tileDeferredPixels;
	std::vector<uint32> m_tileShadedPixels;
	std::vector<std::atomic<LONG> > m_tileReady;
	std::vector<SrRendPrimitve*> m_rendPrimitivePool;
	std::vector<SrRendPrimitve*> m_buildPrimitivePool;
	uint32 m_rendPrimitiveUsed;
	uint32 m_buildPrimitiveUsed;
	SrRendVertex* m_renderVertexBuffer;
	bool m_frameSurfaceReady;
	bool m_presentPending;
	bool m_surfaceReadyForRender;
	bool m_frameRequested;
	bool m_frameComplete;
	bool m_frameInFlight;
	bool m_stopFrameThread;
	std::thread m_frameThread;
	std::mutex m_frameMutex;
	std::condition_variable m_frameRequestCondition;
	std::condition_variable m_frameCompleteCondition;
	std::condition_variable m_surfaceReadyCondition;

	SrTexture* m_MemSBuffer;								///< ��̬��������ڻ���jitAA����ʱ�ĵ�ǰ֡
	SrTexture* m_BackSBuffer;								///< ��̬��������ڻ���jitAA����ʱ����һ֡

	SrRasTaskDispatcher* m_rasTaskDispatcher;				///< Task�ַ�������������̬����
	class SrSoftRenderer* m_renderer;
};

#endif // SrRasterizer_h__

