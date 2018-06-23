/**
  @file prerequisite.h
  
  @brief 前提，所有头文件均引入此文件

  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef prerequisite_h__
#define prerequisite_h__

//////////////////////////////////////////////////////////////////////////
// 全局开关

// 光栅化同步_修正多线程光栅化可能造成的Z冲突
#define RASTERIZER_SYNC
// 固定函数光栅化调用
//#define FIXED_FUNCTION_RASTERIZOR
// SIMD加速
//#define SR_USE_SIMD

// Windows Header Files:
#include <windows.h>

#include <future>

//////////////////////////////////////////////////////////////////////////
// stl
#include <iostream>
// 修改版本的vector以支持内存对齐
#if _MSC_VER <= 1600
#include "myvector"
#else
#include <vector>
#endif
#include <map>
#include <string>
#include <stack>
#include <list>

//////////////////////////////////////////////////////////////////////////
// assert
#if defined( SR_PROFILE ) && defined ( NDEBUG )	// 当在RELEASE模式下，开启SR_PROFILE时，打开assert
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif

// 内存对齐
#define SR_ALIGN _CRT_ALIGN(16)

// fastcall声明
#define SRFASTCALL __fastcall

//////////////////////////////////////////////////////////////////////////
// 工具包
#include "util/math_def.h"
#include "util/pathutil.h"
#include "util/timer.h"
#include "util/thread.h"
#include "util/memfile.h"
#include "util/event.h"


//////////////////////////////////////////////////////////////////////////
// 渲染规格配置
#define SR_MAX_TEXTURE_STAGE_NUM 16
#define FBUFFER_CHANNEL_SIZE 4
#define SR_GREYSCALE_CLEARCOLOR 0x1
#define SR_SHADER_CONSTANTS_NUM 8

// 软件光栅化分块策略
// 此block大小决定了每次线程TASK分发的task个数
// 需要多次测试得到一个较好的值
#define VERTEX_TASK_BLOCK 64			///< VertexShader任务分块大小
#define PIXEL_TASK_BLOCK 64				///< PixelShader任务分块大小
#define RASTERIZE_TASK_BLOCK 512		///< Rasterize任务分块大小

//////////////////////////////////////////////////////////////////////////
// UI显示色彩
#define SR_UICOLOR_HIGHLIGHT 0xffffff
#define SR_UICOLOR_MAIN		 0xaf7333
#define SR_UICOLOR_NORMAL    0xcccccc

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

extern std::map<const void*, void*> m_align_pt_mapper;

inline void* _mm_malloc_16byte(size_t sz, size_t align)
{
	if(sz <= 0)   {
		return NULL;
	}
	unsigned char* pSystemPointer = (unsigned char* )_mm_malloc(sz + 15, 16);
	if(NULL == pSystemPointer)   {
		return NULL;
	}
	size_t offset = 16 - (((unsigned int)pSystemPointer ) % 16);

	m_align_pt_mapper[pSystemPointer + offset] = pSystemPointer;

	return pSystemPointer + offset;
}

inline void _mm_free_16byte(void* p)
{
	_mm_free(m_align_pt_mapper[p]);
}

#define _mm_malloc_custom _mm_malloc_16byte
#define _mm_free_custom _mm_free_16byte

//////////////////////////////////////////////////////////////////////////
// 置前声明
class IRenderer;
struct SrRendPrimitve;
class IProfiler;
struct SrShaderContext;
struct SrMaterial;
class IResourceManager;
class SrTexture;
class SrScene;
class SrSwShader;
class SrResource;
class SrMesh;
class SrShader;
class SrDefaultMediaPack;
struct ILogger;
struct SrRendContext;

typedef std::vector<SrShader*> SrShaderList;
typedef std::vector<HMODULE> SrHandleList;

//////////////////////////////////////////////////////////////////////////
// 全局环境

/**
 *@brief 全局环境，用于全局环境取得主要模块指针
 */
struct GlobalEnvironment
{
	IRenderer*				renderer;
	SrTimer*				timer;
	IProfiler*				profiler;
	IResourceManager*		resourceMgr;
	SrScene*				sceneMgr;
	ILogger*				logger;
	SrRendContext*			context;

	bool					output = false;
};
extern GlobalEnvironment* gEnv;


#include "SrLogger.h"
//	 Simple logs of data with low verbosity.
inline void GtLog( const char* format, ... )
{
	if (gEnv->logger)		
	{
		va_list args;
		va_start(args,format);
		char buffer[1024];
		strcpy(buffer, "#0");
		strcat(buffer, format);
		gEnv->logger->Log( buffer, args );
		va_end(args);
	}
}

inline void GtLogInfo( const char* format, ... )
{
	if (gEnv->logger)		
	{
		va_list args;
		va_start(args,format);
		char buffer[1024];
		strcpy(buffer, "#1");
		strcat(buffer, format);
		gEnv->logger->Log( buffer, args );
		va_end(args);
	}
}

inline void GtLogWarning( const char* format, ... )
{
	if (gEnv->logger)		
	{
		va_list args;
		va_start(args,format);
		char buffer[1024];
		strcpy(buffer, "#2");
		strcat(buffer, format);
		gEnv->logger->Log( buffer, args );
		va_end(args);
	}
}

inline void GtLogError( const char* format, ... )
{
	if (gEnv->logger)		
	{
		va_list args;
		va_start(args,format);
		char buffer[1024];
		strcpy(buffer, "#3");
		strcat(buffer, format);
		gEnv->logger->Log( buffer, args );
		va_end(args);
	}
}

//////////////////////////////////////////////////////////////////////////
// 枚举

/**
 *@brief VB格式, 暂时只使用P3N3T2
 */
enum ESrVertDecl
{
	// data struct ALIGNED
	eVd_Invalid = 0,
	eVd_F4F4,
	eVd_F4F4F4,
	eVd_F4F4F4F4U4,

	eVd_Max,
};

/**
 *@brief 矩阵组
 */
enum EMatrixDefine
{
	eMd_WorldViewProj = 0,
	eMd_World,
	eMd_View,
	eMd_Projection,
	eMd_WorldInverse,
	eMd_ViewInverse,
	eMd_Count,
};

/**
 *@brief 光栅化方式
 */
enum ERasterizeMode
{
	eRm_Solid,
	eRm_WireFrame,
	eRm_Point,
};

/**
 *@brief 采样滤镜
 */
enum ESamplerFilter
{
	eSF_Nearest,		///< 临近点采样
	eSF_Linear,			///< 双线性过滤
};

/**
 *@brief 渲染器状态
 */
enum ERenderingState
{
	eRs_Rendering = 1<<0,
	eRS_Locked = 1<<1,
	eRS_Swaping = 1<<2,
};

/**
 *@brief 渲染特性
 */
enum ERenderFeature
{
	eRFeature_JitAA = 1<<0,						///< 抖动抗锯齿
	eRFeature_MThreadRendering = 1<<1,			///< 多线程渲染
	eRFeature_LinearFiltering = 1<<2,			///< 双线性采样
	eRFeature_DotCoverageRendering = 1<<3,		///< Dot空洞渲染
	eRFeature_InterlaceRendering = 1<<4,		///< Dot空洞渲染
};

/**
 *@brief 资源类型
 */
enum EResourceType
{
	eRt_Mesh = 0,
	eRT_Texture,
	eRT_Material,
	eRT_Shader,

	eRT_Count,
};

enum EShaderConstantsSlot
{
	eSC_VS0 = 0,
	eSC_VS1,
	eSC_VS2,
	eSC_VS3,

	eSC_PS0 = SR_SHADER_CONSTANTS_NUM,
	eSC_PS1,
	eSC_PS2,
	eSC_PS3,

	eSC_ShaderConstantCount = SR_SHADER_CONSTANTS_NUM * 2,
};
//////////////////////////////////////////////////////////////////////////
// 公共渲染结构定义

/**
 *@brief 通用obj模型的顶点格式
 */
SR_ALIGN struct SrVertexP3N3T2
{
	float4 pos;

	// vertex shader usage
	float3 normal;
	float2 texcoord;
};

/**
 *@brief 渲染顶点, DrawPrimitve时利用Primitive构造新顶点
 */
SR_ALIGN struct SrRendVertex
{
	float4 pos;

	// vertex shader usage
	float4 channel1;
	float4 channel2;
	float4 channel3;
	// 	float4 channel6;
	// 	float4 channel7;
};

/**
 *@brief 像素buffer的结构定义
 */
SR_ALIGN struct SrFragment
{
	// preserve a 4-channel data SR_ALIGN struct for GBuffer
	union{
		SR_ALIGN float data[FBUFFER_CHANNEL_SIZE * 4];
		struct  
		{
			float4 hpos;
			float4 worldpos_tx;
			float4 normal_ty;
			float4 preserve;
		};
	};	
	///< 4 x float4 的数据范围
	SrRendPrimitve*	primitive;	///< primitive索引
};

struct SrFragmentBufferSync
{
	SrFragmentBufferSync() {
		InitializeCriticalSection(&cs);
		//InitializeSRWLock(&srwLock);
	}
	~SrFragmentBufferSync() {
		DeleteCriticalSection(&cs);
	}
	CRITICAL_SECTION cs;
	//SRWLOCK srwLock;
};

/**
 *@brief 类型无关的VertexBuffer结构 
 */
SR_ALIGN struct SrVertexBuffer
{
	uint8* data;
	uint32 elementSize;
	uint32 elementCount;

	void* userData;

	SrVertexBuffer()
	{
		memset(this, 0, sizeof(SrVertexBuffer));
	}

	void setData( void* srcData, uint32 index )
	{
		if (index < elementCount)
		{
			memcpy( data + elementSize * index, srcData, elementSize);
		}
	}
};

/**
 *@brief 视口结构
 */
SR_ALIGN struct SrViewport
{
	float x,y,w,h,n,f;

	SrViewport() {}
	SrViewport(float px, float py, float pw, float ph, float pn, float pf) :
		x(px),
		y(py),
		w(pw),
		h(ph),
		n(pn),
		f(pf)
	{

	}
};

/**
 *@brief IndexBuffer结构
 */
SR_ALIGN struct SrIndexBuffer
{
	uint32* data;
	uint32	count;
	uint32  currAddress;

	void* userData;

	SrIndexBuffer()
	{
		data = 0;
		count = 0;
		currAddress = 0;
		userData = 0;
	}
	void setData( uint32 srcData, uint32 index )
	{
		if (index < count)
		{
			data[index] = srcData;
		}
	}

	void Clear()
	{
		memset(data, 0, sizeof(uint32) * count);
		currAddress = 0;
	}

	void push_back(uint32 index)
	{
		data[currAddress++] = index;
		assert( currAddress <= count );
	}

	uint32 size()
	{
		return currAddress;
	}
};

/**
 *@brief 几何体结构
 */
SR_ALIGN struct SrPrimitve
{
	SrVertexBuffer*		vb;			///< vertex buffer
	SrIndexBuffer*		ib;			///< index buffer

	SrVertexBuffer*		cachedVb;

	SrMaterial*			material;	///< 材质

	bool				skined;
};

/**
 *@brief 灯光结构
 */
SR_ALIGN struct SrLight
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 specularColor;

	float3 worldPos;
	float radius;
	float attenuation;

	void * operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void *memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}
};


//////////////////////////////////////////////////////////////////////////
// 结构队列定义
typedef std::vector<SrPrimitve> SrPrimitives;					///< 几何体队列
typedef std::vector<SrLight*> SrLightList;						///< 灯光队列
typedef std::vector<float44> SrMatrixArray;						///< 矩阵队列
typedef std::vector<const SrTexture*> SrBitmapArray;			///< 纹理访问队列
typedef std::vector<SrVertexBuffer*> SrVertexBufferArray;		///< VB队列
typedef std::vector<SrIndexBuffer*> SrIndexBufferArray;			///< IB队列

/**
 *@brief 通用Constant Buffer
 *@remark cBuffer最大只支持8个float4, 即32个浮点数，定义的时候注意大小。
 */
struct SrPixelShader_Constants
{
	float4 perserved0;
	float4 perserved1;
	float4 perserved2;
	float4 perserved3;
	float4 perserved4;
	float4 difColor;
	float4 spcColor;
	float glossness; float fresnelPower; float fresnelBia; float fresnelScale;
};

struct IResourceManager
{
	virtual ~IResourceManager(void) {}

	virtual SrMesh*				LoadMesh(const char* filename) =0;
	virtual const SrTexture*	LoadTexture(const char* filename, bool bump = false) =0;
	virtual SrMaterial*			LoadMaterial(const char* filename) =0;
	virtual SrMaterial*			CreateMaterial(const char* filename) =0;
	virtual void				LoadMaterialLib(const char* filename) =0;
	virtual SrShader*			GetShader(const char* name) =0;
	virtual void				AddShader(SrShader* shader) =0;

	virtual SrTexture*			CreateRenderTexture(const char* name, int width, int height, int bpp) =0;
	virtual SrMaterial*			CreateManmualMaterial(const char* name) =0;

	virtual void				LoadShaderList() =0;

	virtual void				InitDefaultMedia() =0;
	virtual SrDefaultMediaPack*	getDefaultMediaPack() =0;

	// Buffer申请
	virtual SrVertexBuffer* AllocateVertexBuffer(uint32 elementSize, uint32 count, bool fastmode = false) =0;
	virtual bool DeleteVertexBuffer(SrVertexBuffer* target) =0;
	virtual SrIndexBuffer*	AllocateIndexBuffer(uint32 count) =0;
	virtual bool DeleteIndexBuffer(SrIndexBuffer* target) =0;

	virtual void CleanBufferBinding() =0;
};

#endif // prerequisite_h__