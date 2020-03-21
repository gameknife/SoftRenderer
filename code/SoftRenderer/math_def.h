/**
  @file math_def.h
  
  @brief 数学库

  @author yikaiming

  ver:1.0
   
 */

#ifndef math_def_h__
#define math_def_h__

//#define assert( x )  

// PI定义
#define SR_PI 3.1415926f
#define SR_EQUAL_PRECISION 0.001f



// 基础数据类型
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

// 系统数学库引入
#include <math.h>

// namespace srmath
// {
static inline float isqrtf(float x) {return 1.f/sqrtf(x);}

#ifndef OS_UBUNTU
static inline void sincosf (float angle, float* pSin, float* pCos) {	*pSin = float(sin(angle));	*pCos = float(cos(angle));	}
#endif

#ifdef SR_USE_SIMD
// SSE3 including
#include <pmmintrin.h>
// internal Intrinsic Hack
#include "sr_intrinsic_hack.h"
#endif


// 使用RGBA的输出格式
#define SR_COLOR_RGBA

// 引入向量，四元数，矩阵实现
#include "vector.h"
#include "quaternion.h"
#include "matrix.h"



// 模板方法

/**
 @breif 截断
 */
template<typename T>
static inline T Clamp( const T& p , const T& min , const T& max ) { if( p < min ) return min; if( p > max ) return max; return p; }

/**
 @breif float4的单值截断
 */
static inline float4 Clamp(  const float4& p , float min, float max )
{
	
#ifdef SR_USE_SIMD
	float4 ret;
	__m128 max128 = _mm_set_ps1(max);
	__m128 min128 = _mm_set_ps1(min);
	ret.m128 = _mm_locut_ps( _mm_hicut_ps( p.m128, max128 ), min128 );
#else
	float4 ret = p;
	ret.x = Clamp(ret.x, min, max);
	ret.y = Clamp(ret.y, min, max);
	ret.z = Clamp(ret.z, min, max);
	ret.w = Clamp(ret.w, min, max);
#endif
	return ret;
}

/**
 @breif 交换
 */
template<typename T> void SWAP(T& x, T& y)
{
	T tmp = x;
	x = y;
	y = tmp;
}

/**
 @breif 相等判断
 */
static inline bool Equal(float x, float y, float evsilon = SR_EQUAL_PRECISION)
{
	if ( fabsf( x - y ) < evsilon )
	{
		return true;
	}
	return false;
}

/**
 @breif Smooth Hermite插值
 */
static inline float SmoothStep(float mini, float maxi, float x)
{
	float t = Clamp((x - mini) / (maxi - mini), 0.0f, 1.0f);
	return t * t * (3.f - 2.f * t);
}

/**
 @breif Smooth Hermite插值
 */
static inline float SmoothStep(float maxi, float x)
{
	x = Clamp( x / maxi, 0.f, 1.f );
	return  x * x  * (3.0f - 2.0f * x);
}

/**
 @breif ARGB char[4] 转换到 DWORD ARGB
 */
static inline uint32 uint8ARGB_2_uint32( const uint8 * const ch )
{
	uint32 ret = ( ( ch[0] ) << 24 ) |
		( ( ch[1] ) << 16 ) |
		( ( ch[2] ) << 8 ) |
		( ( ch[3] ) );
	return ret;
}

/**
 @breif ARGB char[4] 转换到 DWORD ARGB
 */
static inline uint32 uint32RGB_2_uint32ABGR( const uint32 rgb )
{
	uint32 ret = ( ( rgb & 0xff000000 ) << 24 ) |
		( ( rgb & 0xff0000 ) << 16 ) |
		( ( rgb & 0xff00 ) << 8 ) |
		( ( 0xff ) << 0 );

	return ret;
}

/**
 @breif bmp32 - BGRA 转换到 DWORD ARGB
 */
static inline uint32 uint8BGRA_2_uint32( const uint8 * const ch )
{
	uint32 ret = ( ( ch[0] ) << 0 ) |
		( ( ch[1] ) << 8 ) |
		( ( ch[2] ) << 16) |
		( ( ch[3] ) << 24);
	return ret;
}

/**
 @breif bmp24 - BGR 转换到 DWORD ARGB
 */
static inline uint32 uint8BGR_2_uint32( const uint8 * const ch )
{
	uint32 ret = ( ( ch[0] ) << 0 ) |
		( ( ch[1] ) << 8 ) |
		( ( ch[2] ) << 16) |
		( 0xff << 24);
	return ret;
}

/**
 @breif float4 argb 转换到 DWORD ARGB
 */
static inline uint32 float4_2_uint32( const float4& f )
{
#ifdef SR_USE_SIMD
	__m128 fMul = _mm_set_ps1( 255.f );
	__m128 m128 = _mm_mul_ps( f.m128, fMul );
	__m128i dword128 = _mm_cvttps_epi32(m128);
	uint32* dword4 = (uint32*)(&dword128);

#ifdef SR_COLOR_RGBA
	uint32 ret = ( dword4[0] << 0  )
		| ( dword4[1] << 8 )
		| ( dword4[2] << 16 )
		| ( 0xFF << 24 );
#else
	uint32 ret = ( 0xFF << 24 )
		| ( dword4[1] << 16 )
		| ( dword4[2] << 8 )
		| ( dword4[3] );
#endif

	return ret;
#else

#ifdef SR_COLOR_RGBA
	uint32 ret = ( ( ( uint8 )( f.x * 0xFF ) ) << 0 )
		| ( ( ( uint8 )( f.y * 0xFF ) ) << 8 )
		| ( ( ( uint8 )( f.z * 0xFF ) ) << 16 )
		| ( ( uint8 )( 0xFF ) << 24 );
#else
	uint32 ret = ( ( ( uint8 )( f.x * 0xFF ) ) << 24 )
		| ( ( ( uint8 )( f.y * 0xFF ) ) << 16 )
		| ( ( ( uint8 )( f.z * 0xFF ) ) << 8 )
		| ( ( uint8 )( f.w * 0xFF ) );
#endif
	return ret;
#endif
}

/**
 @breif DWORD ARGB 转换到 float4 argb
 */
static inline float4 uint32_2_float4( uint32 p )
{
#ifdef SR_USE_SIMD
	float4 ret;
	// SSE2 comptible
#ifdef SR_COLOR_RGBA
	__m128i dword4 = _mm_set_epi32( ( p & 0xFF000000 ) >> 24, ( p & 0xFF ) >> 0, ( p & 0xFF00 ) >> 8, ( p & 0xFF0000 ) >> 16 );
#else
	__m128i dword4 = _mm_set_epi32( ( p & 0xFF ) >> 0, ( p & 0xFF00 ) >> 8, ( p & 0xFF0000 ) >> 16,( p & 0xFF000000 ) >> 24 );
#endif

	ret.m128 = _mm_cvtepi32_ps(dword4);
	ret.m128 = _mm_div_ps(ret.m128, _mm_set_ps1(255.f));
	return ret;
#else

#ifdef SR_COLOR_RGBA
	return float4::make(  (float)( ( p & 0xFF0000 ) >> 16 ) / (float)0xFF
		, (float)( ( p & 0xFF00 ) >> 8 ) / (float)0xFF
		, (float)( ( p & 0xFF ) >> 0 )  / (float)0xFF
		,(float)( ( p & 0xFF000000 ) >> 24 ) / (float)0xFF
		);
#else
	return float4( (float)( ( p & 0xFF000000 ) >> 24 ) / (float)0xFF
		, (float)( ( p & 0xFF0000 ) >> 16 ) / (float)0xFF
		, (float)( ( p & 0xFF00 ) >> 8 ) / (float)0xFF
		, (float)( ( p & 0xFF ) ) / (float)0xFF );
#endif
#endif
}

/**
 @breif DWORD ARGB ֱ混合
 */
static inline uint32 SrColorMerge( uint32& color1, uint32& color2, bool avg )
{

#ifdef SR_USE_SIMD
	__m128i a = _mm_set1_epi32(color1);
	__m128i b = _mm_set1_epi32(color2);
	if (avg)
	{
		return _mm_cvtsi128_si32( _mm_avg_epu8(a, b) );
	}
	else
	{
		return _mm_cvtsi128_si32( _mm_add_epi8(a, b) );
	}
	
#else
	return (	((  ((color1 & 0xFF000000) >> 24) + ((color2 & 0xFF000000) >> 24)) / 2u) << 24 |
		((  ((color1 & 0x00FF0000) >> 16) + ((color2 & 0x00FF0000) >> 16)) / 2u) << 16 |
		((  ((color1 & 0x0000FF00) >> 8 ) + ((color2 & 0x0000FF00) >> 8 )) / 2u) << 8 |
		((  ((color1 & 0x000000FF) >> 0 ) + ((color2 & 0x000000FF) >> 0 )) / 2u) << 0 );
#endif // SR_USE_SIMD

}

/**
 @breif DWORD ARGB 乘 floatֵ
 */
static inline uint32 SrColorMulFloat( uint32& color1, float ratio )
{
#ifdef SR_USE_SIMD

	// SSE4
	// unpack һ��u8 u8 u8 u8��i32 i32 i32 i23
	//__m128i dword128 = _mm_cvtepu8_epi32(_mm_set1_epi32(color1));

	// SSE2 comptible
	__m128i dword128 = _mm_setr_epi32( ( color1 & 0xFF ) >> 0, ( color1 & 0xFF00 ) >> 8, ( color1 & 0xFF0000 ) >> 16,( color1 & 0xFF000000 ) >> 24 );

	__m128 float_4 = _mm_mul_ps( _mm_cvtepi32_ps(dword128), _mm_set_ps1(ratio));
	dword128 = _mm_cvttps_epi32(float_4);

	// pack ���Ż�
	uint32* dword4 = (uint32*)(&dword128);
	uint32 ret = ( dword4[0] << 0 )
		| ( dword4[1] << 8 )
		| ( dword4[2] << 16 )
		| ( dword4[3] << 24 );
	return ret;
#else
	return ( (int)(((color1 & 0xFF000000) >> 24) * ratio) << 24 |
		(int)(((color1 & 0x00FF0000) >> 16) * ratio) << 16 |
		(int)(((color1 & 0x0000FF00) >> 8) * ratio) << 8 |
		(int)(((color1 & 0x000000FF) >> 0) * ratio) << 0 		
		);
#endif
}

#ifdef SR_COLOR_RGBA
#define SR_ARGB_F( a, r, g, b ) float4::make( r / 255.f, g / 255.f, b / 255.f, a / 255.f )
#else
#define SR_ARGB_F( a, r, g, b ) float4::make( a / 255.f, r / 255.f, g / 255.f, b / 255.f )
#endif

static inline float SrRandomFloat(float min, float max)
{
	float f01 = (float)(rand() % 1000000) / (float)1000000;
	return (min + f01 * (max - min));
}

// static float GaussianDistribution1D(float x, float rho)
// {
// 	float g = 1.0f / ( rho * sqrtf(2.0f * SR_PI)); 
// 	g *= expf( -(x * x)/(2.0f * rho * rho) );
// 	return g;
// }

/*}*/

#endif // math_def_h__