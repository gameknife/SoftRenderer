/**
  @file prerequisite.h

  @brief ǰ�ᣬ����ͷ�ļ���������ļ�

  @author yikaiming

  ������־ history
  ver:1.0

 */

#ifndef prerequisite_h__
#define prerequisite_h__

//////////////////////////////////////////////////////////////////////////
// ȫ�ֿ���

// ��դ��ͬ��_�������̹߳�դ��������ɵ�Z��ͻ
//#define RASTERIZER_SYNC
// �̶�������դ������
//#define FIXED_FUNCTION_RASTERIZOR
// SIMD����
//#define SR_USE_SIMD

#include "SrPlatform.h"

#include <future>

//////////////////////////////////////////////////////////////////////////
// stl
#include <iostream>
// �޸İ汾��vector��֧���ڴ����
#if defined(_MSC_VER) && _MSC_VER <= 1600
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
#if defined( SR_PROFILE ) && defined ( NDEBUG )	// ����RELEASEģʽ�£�����SR_PROFILEʱ����assert
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif

// �ڴ����
#ifdef _MSC_VER
#define SR_ALIGN _CRT_ALIGN(16)
#else
#define SR_ALIGN
#endif

// fastcall����
#ifdef _MSC_VER
#define SRFASTCALL __fastcall
#endif

//////////////////////////////////////////////////////////////////////////
// ���߰�
#include "util/math_def.h"
#include "util/pathutil.h"
#include "util/timer.h"
#include "util/thread.h"
#include "util/memfile.h"
#include "util/event.h"


//////////////////////////////////////////////////////////////////////////
// ��Ⱦ�������
#define SR_MAX_TEXTURE_STAGE_NUM 16
#define FBUFFER_CHANNEL_SIZE 4
#define SR_GREYSCALE_CLEARCOLOR 0x0
#define SR_SHADER_CONSTANTS_NUM 8

// �����դ���ֿ����
// ��block��С������ÿ���߳�TASK�ַ���task����
// ��Ҫ��β��Եõ�һ���Ϻõ�ֵ
#define VERTEX_TASK_BLOCK 64			///< VertexShader����ֿ��С
#define PIXEL_TASK_BLOCK 64				///< PixelShader����ֿ��С
#define RASTERIZE_TASK_BLOCK 512		///< Rasterize����ֿ��С

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

inline void* _mm_malloc_16byte(size_t sz, size_t align)
{
	if (sz == 0) {
		return NULL;
	}
	// _mm_malloc already guarantees the requested alignment. The previous
	// manual adjustment advanced an already aligned allocation by 16 bytes
	// after reserving only 15 bytes of padding, leaving every object one byte
	// short and corrupting the heap during construction.
#ifdef _MSC_VER
	return _mm_malloc(sz, align);
#else
	void* memory = NULL;
	return posix_memalign(&memory, align, sz) == 0 ? memory : NULL;
#endif
}

inline void _mm_free_16byte(void* p)
{
#ifdef _MSC_VER
	_mm_free(p);
#else
	free(p);
#endif
}

#define _mm_malloc_custom _mm_malloc_16byte
#define _mm_free_custom _mm_free_16byte

//////////////////////////////////////////////////////////////////////////
// ��ǰ����
class IRenderer;
class SrInputManager;
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

typedef std::vector<SrShader*> SrShaderList;
typedef std::vector<HMODULE> SrHandleList;

//////////////////////////////////////////////////////////////////////////
// ȫ�ֻ���

/**
 *@brief ȫ�ֻ���������ȫ�ֻ���ȡ����Ҫģ��ָ��
 */
struct GlobalEnvironment
{
	IRenderer*				renderer;
	SrTimer*				timer;
	SrInputManager*			inputSys;
	IProfiler*				profiler;
	IResourceManager*		resourceMgr;
	SrScene*				sceneMgr;
	ILogger*				logger;
	SrRendContext*			context;
	bool (*present)(HWND window, const void* buffer);
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
// ö��

/**
 *@brief VB��ʽ, ��ʱֻʹ��P3N3T2
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
 *@brief ������
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
 *@brief ��դ����ʽ
 */
enum ERasterizeMode
{
	eRm_Solid,
	eRm_WireFrame,
	eRm_Point,
};

/**
 *@brief �����˾�
 */
enum ESamplerFilter
{
	eSF_Nearest,		///< �ٽ������
	eSF_Linear,			///< ˫���Թ���
};

/**
 *@brief ��Ⱦ��״̬
 */
enum ERenderingState
{
	eRs_Rendering = 1<<0,
	eRS_Locked = 1<<1,
	eRS_Swaping = 1<<2,
};

/**
 *@brief ��Ⱦ����
 */
enum ERenderFeature
{
	eRFeature_JitAA = 1<<0,						///< ���������
	eRFeature_MThreadRendering = 1<<1,			///< ���߳���Ⱦ
	eRFeature_LinearFiltering = 1<<2,			///< ˫���Բ���
	eRFeature_DotCoverageRendering = 1<<3,		///< Dot�ն���Ⱦ
	eRFeature_InterlaceRendering = 1<<4,		///< Dot�ն���Ⱦ
};

/**
 *@brief ��Դ����
 */
static const ERenderFeature eRFeature_TileBinning = static_cast<ERenderFeature>(1 << 5);

enum SrPixelDependencyFlags
{
	ePd_LocalPixel = 0,
	ePd_ReadsFullGBuffer = 1 << 0,
	ePd_ReadsDestination = 1 << 1,
};

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
// ������Ⱦ�ṹ����

/**
 *@brief ͨ��objģ�͵Ķ����ʽ
 */
SR_ALIGN struct SrVertexP3N3T2
{
	float4 pos;

	// vertex shader usage
	float3 normal;
	float2 texcoord;
};

/**
 *@brief ��Ⱦ����, DrawPrimitveʱ����Primitive�����¶���
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
 *@brief ����buffer�Ľṹ����
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
	///< 4 x float4 �����ݷ�Χ
	SrRendPrimitve*	primitive;	///< primitive����
	uint32			drawSequence;
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
 *@brief �����޹ص�VertexBuffer�ṹ
 */
SR_ALIGN struct SrVertexBuffer
{
	uint8* data;
	uint32 elementSize;
	uint32 elementCount;

	void* userData;

	void* operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void* memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}

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
 *@brief �ӿڽṹ
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
 *@brief IndexBuffer�ṹ
 */
SR_ALIGN struct SrIndexBuffer
{
	uint32* data;
	uint32	count;
	uint32  currAddress;

	void* userData;

	void* operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void* memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}

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
 *@brief ������ṹ
 */
SR_ALIGN struct SrPrimitve
{
	SrVertexBuffer*		vb;			///< vertex buffer
	SrIndexBuffer*		ib;			///< index buffer

	SrVertexBuffer*		cachedVb;

	SrMaterial*			material;	///< ����

	bool				skined;
};

/**
 *@brief �ƹ�ṹ
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
// �ṹ���ж���
typedef std::vector<SrPrimitve> SrPrimitives;					///< ���������
typedef std::vector<SrLight*> SrLightList;						///< �ƹ����
typedef std::vector<float44> SrMatrixArray;						///< �������
typedef std::vector<const SrTexture*> SrBitmapArray;			///< ������ʶ���
typedef std::vector<SrVertexBuffer*> SrVertexBufferArray;		///< VB����
typedef std::vector<SrIndexBuffer*> SrIndexBufferArray;			///< IB����

/**
 *@brief ͨ��Constant Buffer
 *@remark cBuffer���ֻ֧��8��float4, ��32���������������ʱ��ע���С��
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

	// Buffer����
	virtual SrVertexBuffer* AllocateVertexBuffer(uint32 elementSize, uint32 count, bool fastmode = false) =0;
	virtual bool DeleteVertexBuffer(SrVertexBuffer* target) =0;
	virtual SrIndexBuffer*	AllocateIndexBuffer(uint32 count) =0;
	virtual bool DeleteIndexBuffer(SrIndexBuffer* target) =0;

	virtual void CleanBufferBinding() =0;
};

#endif // prerequisite_h__
