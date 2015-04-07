/**
  @file sr_intrinsic_hack.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef sr_intrinsic_hack_h__
#define sr_intrinsic_hack_h__

static inline __m128 _mm_locut_ps( __m128 val , __m128 bound )
{
	__m128 mask = _mm_cmplt_ps( val , bound );
	return _mm_or_ps( _mm_and_ps( mask , bound ) , _mm_andnot_ps( mask , val ) );
}
static inline __m128 _mm_hicut_ps( __m128 val , __m128 bound )
{
	__m128 mask = _mm_cmpgt_ps( val , bound );
	return _mm_or_ps( _mm_and_ps( mask , bound ) , _mm_andnot_ps( mask , val ) );
}
static inline __m128 _mm_cross3_ps( __m128 a, __m128 b )
{
	__m128 yzx1 = _mm_shuffle_ps( a, a, _MM_SHUFFLE(3,0,2,1) );
	__m128 zxy1 = _mm_shuffle_ps( a, a, _MM_SHUFFLE(3,1,0,2) );
	__m128 yzx2 = _mm_shuffle_ps( b, b, _MM_SHUFFLE(3,0,2,1) );
	__m128 zxy2 = _mm_shuffle_ps( b, b, _MM_SHUFFLE(3,1,0,2) );
	return _mm_sub_ps( _mm_mul_ps( yzx1, zxy2 ), _mm_mul_ps( zxy1, yzx2) );
}
static inline __m128 _mm_dot_ps( __m128 a , __m128 b ) {
	__m128 s , r;
	s = _mm_mul_ps( a , b );
	r = _mm_add_ss( s , _mm_movehl_ps( s , s ) );
	r = _mm_add_ss( r , _mm_shuffle_ps( r , r , 1 ) );
	return r;
}
#endif // sr_intrinsic_hack_h__