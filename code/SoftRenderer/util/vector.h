/**
  @file vector.h
  
  @brief SoftRenderer数学库：向量类

  @author yikaiming

  更改日志 history
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
	*@brief float2 二维向量类，提供二维向量的基本运算操作。
*@remark 保持为 trivial POD 聚合，以便能作为 float4 等匿名 union 成员（C++ 标准禁止匿名聚合中包含带用户声明构造函数的成员）。使用 float2{a,b} 聚合初始化。
*/
struct float2
{
	union
	{
		float m[2];
		struct { float x; float y; };
	};

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
		float2 tmp{{{ x + p.x , y + p.y }}};
		return tmp;
	}
	inline float2 operator - ( const float2 & p ) const
	{
		float2 tmp{{{ x - p.x , y - p.y }}};
		return tmp;
	}
	inline float operator % ( const float2 & p ) const
	{
		return x * p.y - y * p.x;
	}
	inline float2 operator * ( const float2 & p ) const
	{
		float2 tmp{{{ x * p.x , y * p.y }}};
		return tmp;
	}
	inline float2 operator * ( const float p ) const
	{
		float2 tmp{{{ x * p , y * p }}};
		return tmp;
	}
	inline float2 operator / ( const float p ) const
	{
		float2 tmp{{{ x / p , y / p }}};
		return tmp;
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
	inline float2& reflect( float2& i, float2& n)
	{
		*this= i - n * (i.dot(n)*2.f);
		return *this;
	}
	inline float dot( float2& rhs )
	{
		return x * rhs.x + y * rhs.y;
	}
	static inline float dot( float2& lhs, float2& rhs )
	{
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}
};

/**
	*@brief float3 三维向量类，提供三维向量的基本运算操作。
	*@remark 提供类似 HLSL 的取值和赋值方式，可使用v.xy直接取出前两位

	重载运算符: * 为向量乘法，得数为新向量
			   % 为向量叉程，得数为新向量
			   点积不作运算符重载，使用float3::dot或者.dot来求取点积
*/
SR_ALIGN struct float3
{
	union
	{
		float m[3];
		struct { float x , y , z; };
		struct { float r , g , b; };
		#ifdef SR_USE_SIMD
		__m128 m128;
		#endif
	};

	// 保持为 trivial POD 聚合，以便能作为 float4 等匿名 union 成员。
	// 构造方式：
	//   float3{px, py, pz}         三分量
	//   float3{px, px, px}         广播单值（原 float3(px)）
	//   float3_from_xy_z(xy, z)    由 float2 + z 拼装（原 float3(float2, z)）

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
#else
		float3 tmp{{{ x + p.x , y + p.y , z + p.z }}};
#endif
		return tmp;
	}
	inline float3 operator - ( const float3 & p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_sub_ps( m128, p.m128 );
#else
		float3 tmp{{{ x - p.x , y - p.y , z - p.z }}};
#endif
		return tmp;
	}
	inline float3 operator - (  ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_setzero_ps();
		tmp.m128 = _mm_sub_ps( tmp.m128, m128 );
#else
		float3 tmp{{{ -x , -y , -z }}};
#endif
		return tmp;
	}

	inline float3 operator %= ( const float3 & p )
	{
#ifdef SR_USE_SIMD
 		m128 = _mm_cross3_ps( m128, p.m128 );
#else
		float3 tmp{{{ y * p.z - z * p.y , z * p.x - x * p.z , x * p.y - y * p.x }}};
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
		float3 tmp{{{ y * p.z - z * p.y , z * p.x - x * p.z , x * p.y - y * p.x }}};
#endif
		return tmp;
	}
	inline float3 operator * ( const float3 & p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		tmp.m128 = _mm_mul_ps( m128, p.m128 );
#else
		float3 tmp{{{ x * p.x , y * p.y , z * p.z }}};
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
		float3 tmp{{{ x * p , y * p , z * p }}};
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
		float3 tmp{{{ x / p , y / p , z / p }}};
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
		*this= i - n * (i.dot(n)*2.f);
		return *this;
	}

	inline float dot( const float3& rhs ) const
	{
#ifdef SR_USE_SIMD
	return _mm_cvtss_f32( _mm_dot_ps( m128, rhs.m128 ) );
 #else
	return x * rhs.x + y * rhs.y + z * rhs.z;
#endif
	}

	static inline float dot( const float3& lhs, const float3& rhs )
	{
#ifdef SR_USE_SIMD
		return _mm_cvtss_f32( _mm_dot_ps( lhs.m128, rhs.m128 ) );
#else
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
#endif
	}
	
};


/**
	*@brief float4 四维向量类，提供四维向量的基本运算操作。
	*@remark 四维向量类主要用于 浮点color的操作，存储方式为a,r,g,b
	在math_def中有float4 color和dword color的转换函数

	重载运算符: * 为向量乘法，得数为新向量
	% 为向量叉程，得数为新向量
	点积不作运算符重载，使用float3::dot或者.dot来求取点积
*/
SR_ALIGN struct float4
{
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

	// 保持为 trivial POD 聚合，以便 float3 xyz / float2 xy 等可作为本 union 的成员。
	// 构造方式：
	//   float4{x, y, z, w}            四分量
	//   float4{x, x, x, x}            广播单值（原 float4(px)）
	//   float4_from_v_w(v3, w)        由 float3 + w 拼装（原 float4(float3, w)）
	//   float4_from_ptr(p)            由 float* 加载（原 float4(const float*)）

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
		float4 tmp	{{{ w * p.x + x * p.w - z * p.y + y * p.z
			, w * p.y + y * p.w - x * p.z + z * p.x
			, w * p.z + z * p.w - y * p.x + x * p.y
			, w * p.w - x * p.x - y * p.y - z * p.z }}};
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
		float4 tmp{{{ x + p.x , y + p.y , z + p.z , w + p.w }}};
#endif
		return tmp;
	}
	inline float4 operator - ( const float4 & p ) const
	{
#ifdef SR_USE_SIMD
		float4 tmp;
		tmp.m128 = _mm_sub_ps( m128, p.m128 );
#else
		float4 tmp{{{ x - p.x , y - p.y , z - p.z , w - p.w }}};
#endif		
		return tmp;
	}

	inline float4 operator % ( const float4 & p ) const
	{
		float4 tmp	{{{ w * p.x + x * p.w - z * p.y + y * p.z
			, w * p.y + y * p.w - x * p.z + z * p.x
			, w * p.z + z * p.w - y * p.x + x * p.y
			, w * p.w - x * p.x - y * p.y - z * p.z }}};
		return tmp;
	}
	inline float4 operator * ( const float4 & p ) const
	{
		
#ifdef SR_USE_SIMD
		float4 tmp;
		tmp.m128 = _mm_mul_ps( m128, p.m128 );
#else
		float4 tmp{{{ x * p.x , y * p.y , z * p.z , w * p.w }}};
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
		float4 tmp{{{ x * p , y * p , z * p , w * p }}};
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
		float4 tmp{{{ x / p , y / p , z / p , w / p }}};
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

// 由于 float2/float3/float4 已改为 trivial POD（无用户声明构造函数），
// 提供以下工厂函数替代原先的"组合"构造函数，保持调用点简洁。
// 基本场景请直接使用聚合初始化：float3{a,b,c}、float4{x,y,z,w}。

// 原 float3(const float2& pxy, const float pz)
inline float3 float3_from_xy_z( const float2& pxy, float pz )
{
#ifdef SR_USE_SIMD
	float3 r;
	r.m128 = _mm_set_ps(0, pz, pxy.x, pxy.y);
	return r;
#else
	return float3{{{ pxy.x, pxy.y, pz }}};
#endif
}

// 原 float4(const float3& v, const float pw)
inline float4 float4_from_v_w( const float3& v, float pw )
{
#ifdef SR_USE_SIMD
	float4 r;
	r.m128 = v.m128;
	r.w = pw;
	return r;
#else
	return float4{{{ v.x, v.y, v.z, pw }}};
#endif
}

// 原 float4(const float* p) —— 从内存加载（反转通道顺序以兼容原实现）
inline float4 float4_from_ptr( const float* p )
{
#ifdef SR_USE_SIMD
	float4 r;
	r.m128 = _mm_loadr_ps( p );
	return r;
#else
	return float4{{{ p[0], p[1], p[2], p[3] }}};
#endif
}

// 特化的向量插值

// 快速线性插值
template<class T>
inline T SrFastLerp(const T& a, const T& b, float ratio, float inv_ratio)
{
	return (a * inv_ratio + b * ratio);
}

// 快速线性插值
template<class T>
inline T SrLerp(const T& a, const T& b, float ratio)
{
	T c = a * (1.f - ratio) + b * ratio;
	return c;
}

// 透视矫正插值
template<class T>
inline T SrWLerp(const T& a, const T& b, float ratio, float wa, float wb)
{
	T c = (a * wa * (1.f - ratio) + b * wb * ratio) / ( wb * ratio + wa * (1.f - ratio) );
	return c;
}

// 特化的浮点向量pow操作
static inline float4 pow( const float4& f, float pow)
{
	float4 ret;

	ret.x = powf( f.x, pow );
	ret.y = powf( f.y, pow );
	ret.z = powf( f.z, pow );
	ret.w = powf( f.w, pow );

	return ret;
}

// 特化的浮点向量pow操作
static inline float3 pow( const float3& f, float pow)
{
	float3 ret;
	ret.x = powf( f.x, pow );
	ret.y = powf( f.y, pow );
	ret.z = powf( f.z, pow );

	return ret;
}

#endif // vector_h__
