/**
  @file matrix.h
  
  @brief SoftRenderer数学库：矩阵类

  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef matrix_h__
#define matrix_h__

#define SR_MATRIX33_INDEX( c , r ) ( c * 3 + r )
#define SR_MATRIX44_INDEX( c , r ) ( c * 4 + r )

/**
	*@brief float33 三维矩阵类，提供三维矩阵的基本运算操作。
*/
SR_ALIGN struct float33
{
public:
	union
	{
		float _m[3][3];
		float m[9];
		SR_ALIGN struct{ float m00; float m01; float m02; float m10; float m11; float m12; float m20; float m21; float m22; float m30; float m31; float m32; };
	};
public:

	float33()
	{
	}

	float33( const float * const p )
	{
		for( int i = 0 ; i < 9 ; ++i)
		{
			m[i] = p[i];
		}
	}

	float33(const float3& v1, const float3& v2, const float3& v3 )
	{
		m00 = v1.x; m01 = v1.y; m02 = v1.z;
		m10 = v2.x; m11 = v2.y; m12 = v2.z;
		m20 = v3.x; m21 = v3.y; m22 = v3.z;
	}

	float33( const float p00 , const float p01 , const float p02
		, const float p10 , const float p11 , const float p12
		, const float p20 , const float p21 , const float p22 )
	{
		m[0] = p00;
		m[1] = p01;
		m[2] = p02;
		m[3] = p10;
		m[4] = p11;
		m[5] = p12;
		m[6] = p20;
		m[7] = p21;
		m[8] = p22;
	}

	const float * operator []( const int p ) const 
	{
		return _m[p];
	}

	float33 operator += ( const float33 & p )
	{
		for( int i = 0 ; i < 9 ; ++i)
		{
			m[i] += p.m[i];
		}
		return *this;
	}
	float33 operator -= ( const float33 & p )
	{
		for( int i = 0 ; i < 9 ; ++i)
		{
			m[i] -= p.m[i];
		}
		return *this;
	}
	float33 operator *= ( const float33 & p )
	{
		float33 tmp;
		float sum = 0;
		memset( &tmp , 0 , sizeof( float33 ) );
		for( int i = 0 ; i < 3 ; ++i )
		{
			for( int j = 0 ; j < 3 ; ++j )
			{
				float sum = 0;
				for( int k = 0 ; k < 3 ; ++k )
				{
					sum += m[ SR_MATRIX33_INDEX( i , k ) ] * p.m[ SR_MATRIX33_INDEX( k , j ) ];
				}
				tmp.m[ SR_MATRIX33_INDEX( i , j ) ] = sum;
			}
		}
		for( int i = 0 ; i < 9 ; ++i )
		{
			m[i] = tmp.m[i];
		}

		return *this;
	}
	float3 operator *( const float3 & p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		__m128 line1 = _mm_set_ps( 0, m[6], m[3], m[0] );
		__m128 line2 = _mm_set_ps( 0, m[7], m[4], m[1] );
		__m128 line3 = _mm_set_ps( 0, m[8], m[5], m[2] );

		tmp.x = *((float*)&_mm_dot_ps( p.m128, line1));
		tmp.y = *((float*)&_mm_dot_ps( p.m128, line2));
		tmp.z = *((float*)&_mm_dot_ps( p.m128, line3));
#else
		float3 tmp( p.x * m[0] + p.y * m[3] + p.z * m[6]
		, p.x * m[1] + p.y * m[4] + p.z * m[7]
		, p.x * m[2] + p.y * m[5] + p.z * m[8] );
#endif
		return tmp;
	}		
	bool operator == ( const float33 & p ) const
	{
		for( int i = 0 ; i < 9 ; ++i)
		{
			if( m[i] != p.m[i] )
			{
				return false;
			}
		}
		return true;
	}
	bool operator != ( const float33 & p ) const
	{
		for( int i = 0 ; i < 9 ; ++i)
		{
			if( m[i] != p.m[i] )
			{
				return true;
			}
		}
		return false;
	}
	void Transpose()
	{
		Transpose(*this);
	}

	void Transpose(const float33& M)
	{
		float33 m;
		m.m00=M.m00;		m.m01=M.m10;		m.m02=M.m20;
		m.m10=M.m01;		m.m11=M.m11;		m.m12=M.m21;	
		m.m20=M.m02;		m.m21=M.m12;		m.m22=M.m22;	

		m00=m.m00;		m01=m.m01;		m02=m.m02;
		m10=m.m10;		m11=m.m11;		m12=m.m12;
		m20=m.m20;		m21=m.m21;		m22=m.m22;
	}
};


/**
	*@brief float44 四维矩阵类，提供四维矩阵的基本运算操作，及一些特殊矩阵的构造。
*/
SR_ALIGN struct float44
{
public:
	union
	{
		float _m[4][4];
		float m[16];
		SR_ALIGN struct{ float m00; float m01; float m02; float m03; float m10; float m11; float m12; float m13; float m20; float m21; float m22; float m23; float m30; float m31; float m32; float m33;	};
		struct{__m128 row0, row1, row2, row3;};
		struct{float3 row0_float3, row1_float3, row2_float3, row3_float3;};
		struct{float4 row0_float4, row1_float4, row2_float4, row3_float4;};
	};
public:

	float44()
	{
	}

	float44( const float * const p )
	{
		for( int i = 0 ; i < 16 ; ++i)
		{
			m[i] = p[i];
		}
	}

	float44( const float p00 , const float p01 , const float p02 , const float p03 
		, const float p10 , const float p11 , const float p12 , const float p13
		, const float p20 , const float p21 , const float p22 , const float p23
		, const float p30 , const float p31 , const float p32 , const float p33 )
	{
		m[0] = p00;
		m[1] = p01;
		m[2] = p02;
		m[3] = p03;
		m[4] = p10;
		m[5] = p11;
		m[6] = p12;
		m[7] = p13;
		m[8] = p20;
		m[9] = p21;
		m[10] = p22;
		m[11] = p23;
		m[12] = p30;
		m[13] = p31;
		m[14] = p32;
		m[15] = p33;
	}

	/**
	 *@brief 取元素
	 *@return float * 
	 *@param const int p 
	 */
	const float * operator []( const int p ) const 
	{
		return _m[p];
	}

	/**
	 *@brief 矩阵乘法
	 *@return float44 
	 *@param const float44 & p 
	 */
	float44 operator += ( const float44 & p )
	{
		for( int i = 0 ; i < 16 ; ++i)
		{
			m[i] += p.m[i];
		}
		return *this;
	}
	/**
	 *@brief 矩阵加法
	 *@return float44 
	 *@param const float44 & p 
	 */
	float44 operator -= ( const float44 & p )
	{
		for( int i = 0 ; i < 16 ; ++i)
		{
			m[i] -= p.m[i];
		}
		return *this;
	}
	/**
	 *@brief 矩阵乘法
	 *@return float44 
	 *@param const float44 & p 
	 */
	float44 operator *= ( const float44 & p )
	{
		float44 tmp;
		float sun = 0;
		memset( &tmp , 0 , sizeof( float44 ) );
		for( int i = 0 ; i < 4 ; ++i )
		{
			for( int j = 0 ; j < 4 ; ++j )
			{
				float sum = 0;
				for( int k = 0 ; k < 4 ; ++k )
				{
					sum += m[ SR_MATRIX44_INDEX( i , k ) ] * p.m[ SR_MATRIX44_INDEX( k , j ) ];
				}
				tmp.m[ SR_MATRIX44_INDEX( i , j ) ] = sum;
			}
		}
		for( int i = 0 ; i < 16 ; ++i )
		{
			m[i] = tmp.m[i];
		}

		return *this;
	}
	/**
	 *@brief 矩阵相加
	 *@return float44 
	 *@param const float44 & p 
	 */
	float44 operator + ( const float44 & p ) const
	{
		float44 tmp;
		for( int i = 0 ; i < 16 ; ++i)
		{
			tmp.m[i] = m[i] + p.m[i];
		}
		return tmp;
	}
	/**
	 *@brief 矩阵相减
	 *@return float44 
	 *@param const float44 & p 
	 */
	float44 operator - ( const float44 & p ) const
	{
		float44 tmp;
		for( int i = 0 ; i < 16 ; ++i)
		{
			tmp.m[i] = m[i] - p.m[i];
		}
		return tmp;
	}
	/**
	 *@brief 矩阵相乘
	 *@return float44 
	 *@param const float44 & p 
	 */
	float44 operator * ( const float44 & p ) const
	{
		float44 tmp;
		memset( &tmp , 0 , sizeof( float44 ) );
		for( int i = 0 ; i < 4 ; ++i )
		{
			for( int j = 0 ; j < 4 ; ++j )
			{
				float sum = 0;
				for( int k = 0 ; k < 4 ; ++k )
				{
					sum += m[ SR_MATRIX44_INDEX( i , k ) ] * p.m[ SR_MATRIX44_INDEX( k , j ) ];
				}
				tmp.m[ SR_MATRIX44_INDEX( i , j ) ] = sum;
			}
		}

		return tmp;
	}
	
	/**
	 *@brief 变换4维向量
	 *@return float4 
	 *@param const float4 & p 
	 */
	float4 operator *( const float4 & p ) const
	{
		float4 tmp( p.x * m[0] + p.y * m[4] + p.z * m[8] + p.w * m[12]
		, p.x * m[1] + p.y * m[5] + p.z * m[9] + p.w * m[13]
		, p.x * m[2] + p.y * m[6] + p.z * m[10] + p.w * m[14]
		, p.x * m[3] + p.y * m[7] + p.z * m[11] + p.w * m[15]);
		return tmp;
	}
	/**
	 *@brief 旋转3维向量
	 *@return float3 
	 *@param const float3 & p 
	 */
	float3 operator *( const float3 & p ) const
	{
		float3 tmp( p.x * m[0] + p.y * m[4] + p.z * m[8] + m[12]
		, p.x * m[1] + p.y * m[5] + p.z * m[9] + m[13]
		, p.x * m[2] + p.y * m[6] + p.z * m[10] + m[14] );
		return tmp;
	}		
	/**
	 *@brief 矩阵相似判断
	 *@return bool 
	 *@param const float44 & p 
	 */
	bool operator == ( const float44 & p ) const
	{
		for( int i = 0 ; i < 16 ; ++i)
		{
			if( m[i] != p.m[i] )
			{
				return false;
			}
		}
		return true;
	}
	/**
	 *@brief 矩阵相似判断
	 *@return bool 
	 *@param const float44 & p 
	 */
	bool operator != ( const float44 & p ) const
	{
		for( int i = 0 ; i < 16 ; ++i)
		{
			if( m[i] != p.m[i] )
			{
				return true;
			}
		}
		return false;
	}
	/**
	 *@brief 利用四维矩阵变换向量
	 *@return float3 
	 *@param const float3 & p 
	 */
	float3 MultVector3( const float3 &p )
	{
		float3 tmp( m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12]
		, m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13]
		, m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14] );
		return tmp;
	}
	/**
	 *@brief 利用四维矩阵旋转向量
	 *@return float3 
	 *@param const float3 & p 
	 */
	float3 RotateVector3( const float3 &p ) const
	{
#ifdef SR_USE_SIMD
		float3 tmp;
		__m128 line1 = _mm_set_ps( 0, m[8], m[4], m[0] );
		__m128 line2 = _mm_set_ps( 0, m[9], m[5], m[1] );
		__m128 line3 = _mm_set_ps( 0, m[10], m[6], m[2] );

		tmp.x = *((float*)&_mm_dot_ps( p.m128, line1));
		tmp.y = *((float*)&_mm_dot_ps( p.m128, line2));
		tmp.z = *((float*)&_mm_dot_ps( p.m128, line3));
#else
		float3 tmp( m[0] * p.x + m[4] * p.y + m[8] * p.z
			, m[1] * p.x + m[5] * p.y + m[9] * p.z
			, m[2] * p.x + m[6] * p.y + m[10] * p.z );
#endif

		return tmp;
	}
	/**
	 *@brief 左手系的视图矩阵构造
	 *@return float44 & 
	 *@param float3 & from 
	 *@param float3 & to 
	 *@param float3 & up 
	 */
	float44 &LookatLH( float3 &from , float3 &to , float3 &up )
	{
		*this = CreateIdentity();

		float3 front = ( to - from ).normalize();
		float3 right = ( up % front ).normalize();
		float3 tup = ( front % right ).normalize();
		float3 tfrom = float3( from.dot( right ), from.dot( tup ) , from.dot( front ) );

		_m[0][0] = right.x; _m[1][0] = right.y; _m[2][0] = right.z;
		_m[0][1] = tup.x; _m[1][1] = tup.y; _m[2][1] = tup.z;
		_m[0][2] = front.x; _m[1][2] = front.y; _m[2][2] = front.z;
		_m[3][0] = -tfrom.x; _m[3][1] = -tfrom.y; _m[3][2] = -tfrom.z;

		return *this;
	}
	/**
	 *@brief 右手系的视图矩阵构造
	 *@return float44 & 
	 *@param const float3 & from 
	 *@param const float3 & to 
	 *@param const float3 & up 
	 */
	float44 &LookatRH( const float3 &from , const float3 &to , const float3 &up )
	{
		*this = CreateIdentity();

		float3 front = ( to - from ).normalize();
		float3 right = ( front % up ).normalize();
		float3 tup = ( right % front ).normalize();
		float3 tfrom = float3( float3::dot(from, right) , float3::dot(from, tup) , float3::dot(from, front) );

		_m[0][0] = right.x; _m[1][0] = right.y; _m[2][0] = right.z;
		_m[0][1] = tup.x; _m[1][1] = tup.y; _m[2][1] = tup.z;
		_m[0][2] = front.x; _m[1][2] = front.y; _m[2][2] = front.z;
		_m[3][0] = -tfrom.x; _m[3][1] = -tfrom.y; _m[3][2] = -tfrom.z;
		return *this;
	}
 	/**
 	 *@brief 左手系的透视矩阵构造
 	 *@return float44 & 
 	 *@param float fovy 
 	 *@param float aspect 
 	 *@param float zn 
 	 *@param float zf 
 	 */
 	float44 &PerspectiveFovLH( float fovy , float aspect , float zn , float zf )
	{
		*this = CreateZero();

		float h = 1 / tanf( fovy * 0.5f );
		float w = h / aspect;
		float q , qn;

		q = zf / ( zf - zn );
		qn = -q * zn;

		_m[0][0] = w;
		_m[1][1] = h;

		_m[2][2] = q;
		_m[3][2] = qn;

		_m[2][3] = 1.0f;
		return *this;
	}
	/**
	 *@brief 右手系的透视矩阵构造
	 *@return float44 & 
	 *@param float fovy 
	 *@param float aspect 
	 *@param float zn 
	 *@param float zf 
	 */
	float44 &PerspectiveFovRH( float fovy , float aspect , float zn , float zf )
	{
		*this = CreateZero();

		float h = 1 / tanf( fovy * 0.5f );
		float q = zf / ( zn - zf );
		_m[0][0] = h / aspect;
		_m[1][1] = h;
		_m[2][2] = q;
		_m[3][2] = zn * q;
		_m[2][3] = -1.0f;
		return *this;
	}
	/**
	 *@brief 求转置
	 *@return void 
	 */
	inline void Transpose()
	{
		Transpose(*this);
	}
	/**
	 *@brief 矩阵求转置
	 *@return void 
	 *@param const float44 & M 
	 */
	inline void Transpose(const float44& M)
	{
		float44 m;
		m.m00=M.m00;		m.m01=M.m10;		m.m02=M.m20; 	m.m03=M.m30;
		m.m10=M.m01;		m.m11=M.m11;		m.m12=M.m21; 	m.m13=M.m31;
		m.m20=M.m02;		m.m21=M.m12;		m.m22=M.m22; 	m.m23=M.m32;
		m.m30=M.m03;		m.m31=M.m13;		m.m32=M.m23; 	m.m33=M.m33;
		m00=m.m00;		m01=m.m01;		m02=m.m02;	m03=m.m03;
		m10=m.m10;		m11=m.m11;		m12=m.m12;	m13=m.m13;
		m20=m.m20;		m21=m.m21;		m22=m.m22;	m23=m.m23;
		m30=m.m30;		m31=m.m31;		m32=m.m32;	m33=m.m33;
	}
	inline float44 GetInverse()
	{
		float44	ret = *this;
		ret.Inverse();
		return ret;
	}

	/**
	 *@brief 矩阵求逆
	 *@return void 
	 *@remark 对于正交矩阵，直接使用TRANSPOSE求逆，非常快速
	 */
	inline void Inverse()
	{
		float	tmp[12];
		float44	m=*this;

		tmp[0] = m.m22 * m.m33;
		tmp[1] = m.m32 * m.m23;
		tmp[2] = m.m12 * m.m33;
		tmp[3] = m.m32 * m.m13;
		tmp[4] = m.m12 * m.m23;
		tmp[5] = m.m22 * m.m13;
		tmp[6] = m.m02 * m.m33;
		tmp[7] = m.m32 * m.m03;
		tmp[8] = m.m02 * m.m23;
		tmp[9] = m.m22 * m.m03;
		tmp[10]= m.m02 * m.m13;
		tmp[11]= m.m12 * m.m03;

		m00 = tmp[0]*m.m11 + tmp[3]*m.m21 + tmp[ 4]*m.m31;
		m00-= tmp[1]*m.m11 + tmp[2]*m.m21 + tmp[ 5]*m.m31;
		m01 = tmp[1]*m.m01 + tmp[6]*m.m21 + tmp[ 9]*m.m31;
		m01-= tmp[0]*m.m01 + tmp[7]*m.m21 + tmp[ 8]*m.m31;
		m02 = tmp[2]*m.m01 + tmp[7]*m.m11 + tmp[10]*m.m31;
		m02-= tmp[3]*m.m01 + tmp[6]*m.m11 + tmp[11]*m.m31;
		m03 = tmp[5]*m.m01 + tmp[8]*m.m11 + tmp[11]*m.m21;
		m03-= tmp[4]*m.m01 + tmp[9]*m.m11 + tmp[10]*m.m21;
		m10 = tmp[1]*m.m10 + tmp[2]*m.m20 + tmp[ 5]*m.m30;
		m10-= tmp[0]*m.m10 + tmp[3]*m.m20 + tmp[ 4]*m.m30;
		m11 = tmp[0]*m.m00 + tmp[7]*m.m20 + tmp[ 8]*m.m30;
		m11-= tmp[1]*m.m00 + tmp[6]*m.m20 + tmp[ 9]*m.m30;
		m12 = tmp[3]*m.m00 + tmp[6]*m.m10 + tmp[11]*m.m30;
		m12-= tmp[2]*m.m00 + tmp[7]*m.m10 + tmp[10]*m.m30;
		m13 = tmp[4]*m.m00 + tmp[9]*m.m10 + tmp[10]*m.m20;
		m13-= tmp[5]*m.m00 + tmp[8]*m.m10 + tmp[11]*m.m20;

		tmp[ 0] = m.m20*m.m31;
		tmp[ 1] = m.m30*m.m21;
		tmp[ 2] = m.m10*m.m31;
		tmp[ 3] = m.m30*m.m11;
		tmp[ 4] = m.m10*m.m21;
		tmp[ 5] = m.m20*m.m11;
		tmp[ 6] = m.m00*m.m31;
		tmp[ 7] = m.m30*m.m01;
		tmp[ 8] = m.m00*m.m21;
		tmp[ 9] = m.m20*m.m01;
		tmp[10] = m.m00*m.m11;
		tmp[11] = m.m10*m.m01;

		m20 = tmp[ 0]*m.m13 + tmp[ 3]*m.m23 + tmp[ 4]*m.m33;
		m20-= tmp[ 1]*m.m13 + tmp[ 2]*m.m23 + tmp[ 5]*m.m33;
		m21 = tmp[ 1]*m.m03 + tmp[ 6]*m.m23 + tmp[ 9]*m.m33;
		m21-= tmp[ 0]*m.m03 + tmp[ 7]*m.m23 + tmp[ 8]*m.m33;
		m22 = tmp[ 2]*m.m03 + tmp[ 7]*m.m13 + tmp[10]*m.m33;
		m22-= tmp[ 3]*m.m03 + tmp[ 6]*m.m13 + tmp[11]*m.m33;
		m23 = tmp[ 5]*m.m03 + tmp[ 8]*m.m13 + tmp[11]*m.m23;
		m23-= tmp[ 4]*m.m03 + tmp[ 9]*m.m13 + tmp[10]*m.m23;
		m30 = tmp[ 2]*m.m22 + tmp[ 5]*m.m32 + tmp[ 1]*m.m12;
		m30-= tmp[ 4]*m.m32 + tmp[ 0]*m.m12 + tmp[ 3]*m.m22;
		m31 = tmp[ 8]*m.m32 + tmp[ 0]*m.m02 + tmp[ 7]*m.m22;
		m31-= tmp[ 6]*m.m22 + tmp[ 9]*m.m32 + tmp[ 1]*m.m02;
		m32 = tmp[ 6]*m.m12 + tmp[11]*m.m32 + tmp[ 3]*m.m02;
		m32-= tmp[10]*m.m32 + tmp[ 2]*m.m02 + tmp[ 7]*m.m12;
		m33 = tmp[10]*m.m22 + tmp[ 4]*m.m02 + tmp[ 9]*m.m12;
		m33-= tmp[ 8]*m.m12 + tmp[11]*m.m22 + tmp[ 5]*m.m02;

		float det=(m.m00*m00+m.m10*m01+m.m20*m02+m.m30*m03);

		float idet=(float)1.0/det;
		m00*=idet; m01*=idet; m02*=idet; m03*=idet;
		m10*=idet; m11*=idet; m12*=idet; m13*=idet;
		m20*=idet; m21*=idet; m22*=idet; m23*=idet;
		m30*=idet; m31*=idet; m32*=idet; m33*=idet;
	}


	/**
	 *@brief 利用scale, rotation, position构造新的世界矩阵
	 *@return void 
	 *@param const float3 & s 
	 *@param const Quat & q 
	 *@param const float3 & t 
	 */
	void float44::Set( const float3& s, const Quat& q, const float3& t ) {
		float vxvx=q.v.x*q.v.x;					float vzvz=q.v.z*q.v.z;					float vyvy=q.v.y*q.v.y; 
		float vxvy=q.v.x*q.v.y;					float vxvz=q.v.x*q.v.z;					float vyvz=q.v.y*q.v.z; 
		float svx=q.w*q.v.x;					float svy=q.w*q.v.y;					float svz=q.w*q.v.z;
		m00=(1-(vyvy+vzvz)*2)*s.x;			m10=(vxvy-svz)*2*s.y;				m20=(vxvz+svy)*2*s.z;				m30=t.x;
		m01=(vxvy+svz)*2*s.x;				m11=(1-(vxvx+vzvz)*2)*s.y;			m21=(vyvz-svx)*2*s.z;				m31=t.y;
		m02=(vxvz-svy)*2*s.x;				m12=(vyvz+svx)*2*s.y;				m22=(1-(vxvx+vyvy)*2)*s.z;			m32=t.z;
	}

	/**
	 *@brief 取得行优先的矩阵的位移值
	 *@return float3 
	 */
	float3 float44::GetTranslate() const
	{
#ifdef SR_USE_SIMD
		return float3(m30, m31, m32);
		//return row3_float3;
#else
		return float3(m30, m31, m32);
#endif
	}

	/**
	 *@brief 静态函数：构造单位矩阵
	 *@return float44 
	 */
	static float44 float44::CreateIdentity()
	{
		return float44( 1 , 0 , 0 , 0 , 0 , 1 , 0 , 0 , 0 , 0 , 1 , 0, 0 , 0 , 0 , 1 );
	}

	/**
	 *@brief 静态函数：构造0矩阵
	 *@return float44 
	 */
	static float44 float44::CreateZero()
	{
		return float44( 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0, 0 , 0 , 0 , 0 );
	}
};

typedef float44 float44;
typedef float33 float33;

#endif