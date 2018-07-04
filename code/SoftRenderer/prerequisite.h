/**
  @file prerequisite.h
  
  @brief 前提，所有头文件均引入此文件

  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef prerequisite_h__
#define prerequisite_h__

// 平台判断

#if defined(__GNUG__)
# define COMPILER_GCC
#elif defined(_MSC_VER)
# define COMPILER_MSVC
 // i don't care about your debug symbol issues...
# pragma warning(disable:4786)
#else
# error "Could not determine compiler"
#endif

#if defined( __SYMBIAN32__ ) 
#   define OS_SYMBIAN
#elif defined( __WIN32__ ) || defined( _WIN32 )
#   define OS_WIN32
#elif defined( __APPLE_CC__)
#   if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 40000 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000
#       define OS_IOS
#   else
#       define OS_IOS
#       define OS_APPLE
#   endif
#elif defined(__ANDROID__)
#	define OS_ANDROID
#else
#	error "Could not determine OS"
#endif

#if defined (OS_ANDROID) || defined( OS_IOS ) || defined( OS_APPLE )
#	define OS_LINUX
#endif


//////////////////////////////////////////////////////////////////////////
// 全局开关

// 光栅化同步_修正多线程光栅化可能造成的Z冲突
#define RASTERIZER_SYNC
// 固定函数光栅化调用
//#define FIXED_FUNCTION_RASTERIZOR
// SIMD加速
//#define SR_USE_SIMD




// Windows Header Files:
#ifdef OS_WIN32
#include <windows.h>
#endif

// SIMD
#ifdef SR_USE_SIMD
#include <future>
#endif

//////////////////////////////////////////////////////////////////////////
// stl
#include <iostream>
#include <vector>
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
#ifdef OS_WIN32
#define SR_ALIGN _CRT_ALIGN(16)
#else
#define SR_ALIGN
#endif

// fastcall声明
#define SRFASTCALL __fastcall

//////////////////////////////////////////////////////////////////////////
// 工具包
#include "math_def.h"
#include "pathutil.h"
#include "timer.h"
#include "memfile.h"

#include "IEvent.h"
#include "IThread.h"
#include "mmalloc.h"



//////////////////////////////////////////////////////////////////////////
// 渲染规格配置
#define SR_MAX_TEXTURE_STAGE_NUM 16
#define FBUFFER_CHANNEL_SIZE 4
#define SR_GREYSCALE_CLEARCOLOR 0x0
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

//////////////////////////////////////////////////////////////////////////
// 置前声明
class IRenderer;
struct SrRendPrimitve;
struct IProfiler;
struct SrShaderContext;
struct SrMaterial;
struct IResourceManager;
class SrTexture;
class SrScene;
class SrSwShader;
class SrResource;
class SrMesh;
class SrShader;
struct SrDefaultMediaPack;
struct ILogger;
struct SrRendContext;
class SrEntity;
struct SrCamera;

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

#include "srtype.h"


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
		//InitializeCriticalSection(&cs);
		//InitializeSRWLock(&srwLock);
	}
	~SrFragmentBufferSync() {
		//DeleteCriticalSection(&cs);
	}
	//CRITICAL_SECTION cs;
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