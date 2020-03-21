/**
  @file vector.h
  
  @brief SoftRenderer��ѧ�⣺������

  @author yikaiming

  history
  ver:1.0
   
 */

#ifndef vector_h__
#define vector_h__

class int2
{
public:
	union
	{
		int m[2];
		struct { int x; int y; };
	};

	int2()
	{
	}

	int2( const int * const p )
	{
		x = p[0];
		y = p[1];
	}

	int2( const int px , const int py )
	{
		x = px;
		y = py;
	}
};
/**
	*@brief float2 ��ά�����࣬�ṩ��ά�����Ļ������������
*/
struct float2
{
public:
	union
	{
		float m[2];
		struct { float x; float y; };
	};

	static float2 make(const float * const p)
	{
		float2 ret;
		ret.x = p[0];
		ret.y = p[1];
		return ret;
	}

	static float2 make( const float px , const float py )
	{
		float2 ret;
		ret.x = px;
		ret.y = py;
		return ret;
	}

	// float2( const float * const p )
	// {
	// 	x = p[0];
	// 	y = p[1];
	// }

	// float2( const float px , const float py )
	// {
	// 	x = px;
	// 	y = py;
	// }

	inline float2 & operator += ( const float2 & p )
	{
		x += p.x;
		y += p.y;
		return (*this);
	}
	inline float2 & operator -= ( const float2 & p )
	{
		x -= p.x;
		y -= p.y;
		return (*this);
	}
	inline float2 & operator *= ( const float2& p )
	{
		x *= p.x;
		y *= p.y;
		return (*this);
	}
	inline float2 & operator *= ( const float p )
	{
		x *= p;
		y *= p;
		return (*this);
	}
	inline float2 & operator /= ( const float p )
	{
		x /= p;
		y /= p;
		return (*this);
	}

	inline float2 operator + ( const float2 & p ) const
	{
		return float2::make( x + p.x , y + p.y );
	}
	inline float2 operator - ( const float2 & p ) const
	{
		return float2::make( x - p.x , y - p.y );
	}
	inline float operator % ( const float2 & p ) const
	{
		return x * p.y - y * p.x;
	}
	inline float2 operator * ( const float2 & p ) const
	{
		return float2::make( x * p.x , y * p.y );
	}
	inline float2 operator * ( const float p ) const
	{
		return float2::make( x * p , y * p );
	}
	inline float2 operator / ( const float p ) const
	{
		return float2::make( x / p , y / p );
	}

	bool operator == ( const float2 & p ) const
	{
		if( x + SR_EQUAL_PRECISION < p.x || x - SR_EQUAL_PRECISION > p.x )
		{
			return false;
		}
		if( y + SR_EQUAL_PRECISION < p.y || y - SR_EQUAL_PRECISION > p.y )
		{
			return false;
		}
		return true;
	}
	bool operator != ( const float2 & p ) const
	{
		if( x + SR_EQUAL_PRECISION < p.x || x - SR_EQUAL_PRECISION > p.x )
		{
			return true;
		}
		if( y + SR_EQUAL_PRECISION < p.y || y - SR_EQUAL_PRECISION > p.y )
		{
			return true;
		}
		return false;
	}

	inline float length(  ) const
	{
		return sqrt( x * x + y * y );
	}
	inline float lengthsq(  ) const
	{
		return x * x + y * y;
	}
	inline float2 & normalize(  )
	{
		(*this) /= length();
		return (*this);
	}

	inline void zero()
	{
		memset(this, 0, sizeof(float2));
	}

	inline void set( float nx, float ny )
	{
		x = nx;
		y = ny;
	}
	inline float2& reflect( const float2& i, const float2& n)
	{
		*this= i - n * (float2::dot(i, n)*2.f);
		return *this;
	}
	inline float dot( const float2& rhs ) const
	{
		return x * rhs.x + y * rhs.y;
	}
	static inline float dot( const float2& lhs, const float2& rhs )
	{
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}
};

/**
	*@brief float3 
	hlsl like vector function
*/
SR_ALIGN struct float3
{
public:
	union
	{
		float m[3];
		struct { float x , y , z; };
		struct { float r , g , b; };
		struct { float2 xy; float vz;};
#ifdef SR_USE_SIMD
		__m128 m128;
#endif
	};

	static float3 make( const float px , const float py , const float pz )
	{
		float3 ret;
// #ifdef SR_USE_SIMD
// 		m128 = _mm_set_ps(0, pz, py, px);
// #else
		ret.x = px;
		ret.y = py;
		ret.z = pz;
//#endif
		return ret;
	}

	static float3 make( const float2& pxy , const float pz )
	{
		float3 ret;
#ifdef SR_USE_SIMD
		ret.m128 = _mm_set_ps(0, pz, pxy.x, pxy.y);
#else
		ret.x = pxy.x;
		ret.y = pxy.y;
		ret.z = pz;
#endif
		return ret;
	}

	static float3 make( const float px)
	{
		float3 ret;
#ifdef SR_USE_SIMD
		ret.m128 = _mm_set_ps1(px);
#else
		ret.x = px;
		ret.y = px;
		ret.z = px;
#endif
		return ret;
	}

	inline float3 & operator += ( const float3 & p )
	{
#ifdef SR_USE_SIMD
		m128 = _mm_add_ps( m128, p.m128 );
#else
		x += p.x;
		y += p.y;
		z += p.z;
#endif
		return (*this);
	}
	inline float3 & operator -= ( const float3 & p )
	{
#ifdef SR_USE_SIMD
		m128 = _mm_sub_ps( m128, p.m128 );
#else
		x -= p.x;
		y -= p.y;
		z -= p.z;
#endif
		return (*this);
	}
	inline float3 operator *= ( const float3 & p )
	{
#ifdef SR_USE_SIMD
		m128 = _mm_mul_ps( m128, p.m128 );
#else
		x *= p.x;
		y *= p.y;
		z *= p.z;
#endif
		return (*this);
	}
	inline float3 & operator *= ( const float p )
	{
#ifdef SR_USE_SIMD
		__m128 div = _mm_set_ps1(p);
		m128 = _mm_mul_ps( m128, div );
#else
		x *= p;
		y *= p;
		z *= p;
#endif
		return (*this);
	}
	inline float3 & operator /= ( const float p )
	{
#ifdef SR_USE_SIMD
		__m128 div = _mm_set_ps1(p);
		m128 = _mm_div_ps( m128, div );
#else
		x /= p;
		y /= p;
		z /= p;
#endif
		return (*this);
	}

	inline float3 operator + ( const float3 & p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_add_ps( m128, p.m128 );
		return tmp;
#else
		return float3::make( x + p.x , y + p.y , z + p.z );
#endif
	}
	inline float3 operator - ( const float3 & p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_sub_ps( m128, p.m128 );
#else
		float3 tmp = float3::make( x - p.x , y - p.y , z - p.z );
#endif
		return tmp;
	}
	inline float3 operator - (  ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_setzero_ps();
		tmp.m128 = _mm_sub_ps( tmp.m128, m128 );
		return tmp;
#else
		return float3::make( -x , -y , -z );
#endif
	}

	inline float3 operator %= ( const float3 & p )
	{
#ifdef SR_USE_SIMD
 		m128 = _mm_cross3_ps( m128, p.m128 );
#else
		float3 tmp = float3::make( y * p.z - z * p.y , z * p.x - x * p.z , x * p.y - y * p.x );
		x = tmp.x;
		y = tmp.y;
		z = tmp.z;
#endif
		return (*this);
	}
	inline float3 operator % ( const float3 & p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
 		tmp.m128 = _mm_cross3_ps( m128, p.m128 );
 #else
		float3 tmp = float3::make( y * p.z - z * p.y , z * p.x - x * p.z , x * p.y - y * p.x );
#endif
		return tmp;
	}
	inline float3 operator * ( const float3 & p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_mul_ps( m128, p.m128 );
#else
		float3 tmp = float3::make( x * p.x , y * p.y , z * p.z );
#endif
		return tmp;
	}

	inline float3 operator * ( const float p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_set_ps1(p);
		tmp.m128 = _mm_mul_ps( m128, tmp.m128 );
#else
		float3 tmp = float3::make( x * p , y * p , z * p );
#endif
		return tmp;
	}
	inline float3 operator / ( const float p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_set_ps1(p);
		tmp.m128 = _mm_div_ps( m128, tmp.m128 );
#else
		float3 tmp = float3::make( x / p , y / p , z / p );
#endif
		return tmp;
	}

	bool operator == ( const float3 & p ) const
	{
		if( x + SR_EQUAL_PRECISION < p.x || x - SR_EQUAL_PRECISION > p.x )
		{
			return false;
		}
		if( y + SR_EQUAL_PRECISION < p.y || y - SR_EQUAL_PRECISION > p.y )
		{
			return false;
		}
		if( z + SR_EQUAL_PRECISION < p.z || z - SR_EQUAL_PRECISION > p.z )
		{
			return false;
		}
		return true;
	}
	bool operator != ( const float3 & p ) const
	{
		if( x + SR_EQUAL_PRECISION < p.x || x - SR_EQUAL_PRECISION > p.x )
		{
			return true;
		}
		if( y + SR_EQUAL_PRECISION < p.y || y - SR_EQUAL_PRECISION > p.y )
		{
			return true;
		}
		if( z + SR_EQUAL_PRECISION < p.z || z - SR_EQUAL_PRECISION > p.z )
		{
			return true;
		}
		return true;
	}
	inline float lengthsq(  ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_mul_ps( m128, m128 );
		return tmp.x + tmp.y + tmp.z;
#else
		return x * x + y * y + z * z;
#endif
	}

	inline float length(  ) const
	{
		return sqrt( lengthsq() );
	}

	inline float3 & normalize(  )
	{
		(*this) /= length();
		return (*this);
	}

	inline float3& reflect( const float3& i, const float3& n)
	{
		*this= i - n * (float3::dot(i,n)*2.f);
		return *this;
	}

	inline float dot( const float3& rhs )
	{
#ifdef SR_USE_SIMD
	__m128 value = _mm_dot_ps( m128, rhs.m128 );
	return *(reinterpret_cast<float*>(&value));
 #else
	return x * rhs.x + y * rhs.y + z * rhs.z;
#endif
	}

	static inline float dot( const float3& lhs, const float3& rhs )
	{
#ifdef SR_USE_SIMD
		__m128 value = _mm_dot_ps( lhs.m128, rhs.m128 );
		return *(reinterpret_cast<float*>(&value));
#else
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
#endif
	}
	
};


/**
	*@brief float4 ��ά�����࣬�ṩ��ά�����Ļ������������
	*@remark ��ά��������Ҫ���� ����color�Ĳ������洢��ʽΪa,r,g,b
	��math_def����float4 color��dword color��ת������

	���������: * Ϊ�����˷�������Ϊ������
	% Ϊ������̣�����Ϊ������
	���������������أ�ʹ��float3::dot����.dot����ȡ���
*/
SR_ALIGN struct float4
{
public:
	union
	{
		SR_ALIGN float m[4];

		SR_ALIGN struct { float x , y , z , w; };
#ifdef SR_COLOR_RGBA
		SR_ALIGN struct { float r , g , b , a; };
#else
		SR_ALIGN struct { float a , r , g , b; };
#endif
		SR_ALIGN struct { 
			float3 xyz;
		};
		SR_ALIGN struct { 
			float3 rgb;
		};
		SR_ALIGN struct { 
			float2 xy; float2 zw;
		};

#ifdef SR_USE_SIMD
		__m128 m128;
#endif
	};

	// float4()
	// {
	// }

	static float4 make( const float * const p )
	{
		float4 tmp;
#ifdef SR_USE_SIMD
 		tmp.m128 = _mm_loadr_ps( p );
#else
		tmp.x = p[0];
		tmp.y = p[1];
		tmp.z = p[2];
		tmp.w = p[3];
#endif
		return tmp;
	}

	static float4 make( const float px , const float py , const float pz , const float pw )
	{
		float4 tmp;
// #ifdef SR_USE_SIMD
// 		m128 = _mm_set_ps(pw, pz, py, px);
// #else
		tmp.x = px;
		tmp.y = py;
		tmp.z = pz;
		tmp.w = pw;
//#endif
		return tmp;
	}

	static float4 make( const float px )
	{
		float4 tmp;
#ifdef SR_USE_SIMD
		tmp.m128 = _mm_set_ps1(px);
#else
		tmp.x = px;tmp.y = px;tmp.z = px;tmp.w = px;
#endif
		return tmp;
	}

	static float4 make( const float3& v, const float pw )
	{
		float4 tmp;
#ifdef SR_USE_SIMD
			tmp.m128 = v.m128;
			tmp.w = pw;
#else
			tmp.x = v.x;tmp.y = v.y; tmp.z = v.z; tmp.w = pw;
#endif
		return tmp;
	}

	inline float4 & operator += ( const float4 & p )
	{
#ifdef SR_USE_SIMD
		m128 = _mm_add_ps( m128, p.m128 );
#else
		x += p.x;
		y += p.y;
		z += p.z;
		w += p.w;
#endif
		return (*this);
	}
	inline float4 & operator -= ( const float4 & p )
	{
#ifdef SR_USE_SIMD
		m128 = _mm_sub_ps( m128, p.m128 );
#else
		x -= p.x;
		y -= p.y;
		z -= p.z;
		w -= p.w;
#endif
		return (*this);
	}
	inline float4 & operator %= ( const float4 & p )
	{
		float4 tmp = float4::make( w * p.x + x * p.w - z * p.y + y * p.z
			, w * p.y + y * p.w - x * p.z + z * p.x
			, w * p.z + z * p.w - y * p.x + x * p.y
			, w * p.w - x * p.x - y * p.y - z * p.z );
		x = tmp.x;
		y = tmp.y;
		z = tmp.z;
		w = tmp.w;
		return (*this);
	}
	inline float4 & operator *= ( const float4 & p )
	{
#ifdef SR_USE_SIMD
		m128 = _mm_mul_ps( m128, p.m128 );
#else
		x *= p.x;
		y *= p.y;
		z *= p.z;
		w *= p.w;
#endif
		return (*this);
	}
	inline float4 & operator *= ( const float p )
	{
#ifdef SR_USE_SIMD
		__m128 mul = _mm_set_ps1(p);
		m128 = _mm_mul_ps( m128, mul );
#else
		x *= p;
		y *= p;
		z *= p;
		w *= p;
#endif

		return (*this);
	}
	inline float4 & operator /= ( const float p )
	{
#ifdef SR_USE_SIMD
		__m128 div = _mm_set_ps1(p);
		m128 = _mm_div_ps(m128, div);
#else
		x /= p;
		y /= p;
		z /= p;
		w /= p;
#endif		

		return (*this);
	}

	inline float4 operator + ( const float4 & p ) const
	{
#ifdef SR_USE_SIMD
		float4 tmp;
		tmp.m128 = _mm_add_ps( m128, p.m128 );
#else
		float4 tmp = float4::make( x + p.x , y + p.y , z + p.z , w + p.w );
#endif
		return tmp;
	}
	inline float4 operator - ( const float4 & p ) const
	{
#ifdef SR_USE_SIMD
		float4 tmp;
		tmp.m128 = _mm_sub_ps( m128, p.m128 );
#else
		float4 tmp = float4::make( x - p.x , y - p.y , z - p.z , w - p.w );
#endif		
		return tmp;
	}

	inline float4 operator % ( const float4 & p ) const
	{
		float4 tmp = float4::make( w * p.x + x * p.w - z * p.y + y * p.z
			, w * p.y + y * p.w - x * p.z + z * p.x
			, w * p.z + z * p.w - y * p.x + x * p.y
			, w * p.w - x * p.x - y * p.y - z * p.z );
		return tmp;
	}
	inline float4 operator * ( const float4 & p ) const
	{
		
#ifdef SR_USE_SIMD
		float4 tmp;
		tmp.m128 = _mm_mul_ps( m128, p.m128 );
#else
		float4 tmp = float4::make( x * p.x , y * p.y , z * p.z , w * p.w );
#endif
		return tmp;
	}

	inline float4 operator * ( const float p ) const
	{
		#ifdef SR_USE_SIMD
		float4 tmp;
		tmp.m128 = _mm_set_ps1(p);
		tmp.m128 = _mm_mul_ps( m128, tmp.m128 );
		#else
		float4 tmp = float4::make( x * p , y * p , z * p , w * p );
		#endif
		
		return tmp;
	}
	inline float4 operator / ( const float p ) const
	{
#ifdef SR_USE_SIMD
		float4 tmp;
		tmp.m128 = _mm_set_ps1(p);
		tmp.m128 = _mm_div_ps( m128, tmp.m128 );
#else
		float4 tmp = float4::make( x / p , y / p , z / p , w / p );
#endif		
		return tmp;
	}

	bool operator == ( const float4 & p ) const
	{
		if( x + SR_EQUAL_PRECISION < p.x || x - SR_EQUAL_PRECISION > p.x )
		{
			return false;
		}
		if( y + SR_EQUAL_PRECISION < p.y || y - SR_EQUAL_PRECISION > p.y )
		{
			return false;
		}
		if( z + SR_EQUAL_PRECISION < p.z || z - SR_EQUAL_PRECISION > p.z )
		{
			return false;
		}
		if( w + SR_EQUAL_PRECISION < p.w || w - SR_EQUAL_PRECISION > p.w )
		{
			return false;
		}
		return true;
	}
	bool operator != ( const float4 & p ) const
	{
		if( x + SR_EQUAL_PRECISION < p.x || x - SR_EQUAL_PRECISION > p.x )
		{
			return true;
		}
		if( y + SR_EQUAL_PRECISION < p.y || y - SR_EQUAL_PRECISION > p.y )
		{
			return true;
		}
		if( z + SR_EQUAL_PRECISION < p.z || z - SR_EQUAL_PRECISION > p.z )
		{
			return true;
		}
		if( w + SR_EQUAL_PRECISION < p.w || w - SR_EQUAL_PRECISION > p.w )
		{
			return true;
		}
		return true;
	}

	inline float length(  ) const
	{
		return sqrtf( x * x + y * y + z * z + w * w );
	}
	inline float lengthsq(  ) const
	{
		return x * x + y * y + z * z + w * w;
	}
	inline float4 & normalize(  )
	{
		(*this) /= length();
		return (*this);
	}
	inline void sqrt()
	{
#ifdef SR_USE_SIMD
		m128 = _mm_sqrt_ps( m128);
#else
		x = sqrtf(x);
		y = sqrtf(y);
		z = sqrtf(z);
		w = sqrtf(w);
#endif
	}
	inline float dot( float4& rhs )
	{
		return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
	}
	static inline float dot( float4& lhs, float4& rhs )
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
	}
};

// �ػ���������ֵ

// �������Բ�ֵ
template<class T>
inline T SrFastLerp(const T& a, const T& b, float ratio, float inv_ratio)
{
	return (a * inv_ratio + b * ratio);
}

// �������Բ�ֵ
template<class T>
inline T SrLerp(const T& a, const T& b, float ratio)
{
	T c = a * (1.f - ratio) + b * ratio;
	return c;
}

// ͸�ӽ�����ֵ
template<class T>
inline T SrWLerp(const T& a, const T& b, float ratio, float wa, float wb)
{
	T c = (a * wa * (1.f - ratio) + b * wb * ratio) / ( wb * ratio + wa * (1.f - ratio) );
	return c;
}

// �ػ��ĸ�������pow����
static inline float4 pow( const float4& f, float pow)
{
	float4 ret;

	ret.x = powf( f.x, pow );
	ret.y = powf( f.y, pow );
	ret.z = powf( f.z, pow );
	ret.w = powf( f.w, pow );

	return ret;
}

// �ػ��ĸ�������pow����
static inline float3 pow( const float3& f, float pow)
{
	float3 ret;
	ret.x = powf( f.x, pow );
	ret.y = powf( f.y, pow );
	ret.z = powf( f.z, pow );

	return ret;
}

#endif // vector_h__