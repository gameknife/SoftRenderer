/**
  @file quaternion.h
  
  @brief SoftRenderer数学库：四元数类

  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef quaternion_h__
#define quaternion_h__

struct Quat;

float operator | (const Quat& q, const Quat& p);

SR_ALIGN struct Quat
{
 	union
 	{
		SR_ALIGN struct{float3 v; float w;};
 		SR_ALIGN struct{ float x , y , z, w; };
 	};

	 Quat() {}

	 Quat( float W, float X, float Y, float Z, bool bNorm = false ) 
		: w(W),v(X,Y,Z)	
	{
		if (bNorm) Normalize();
		assert(IsValid()); 
	}
	 Quat( float angle, const float3 &axis) : w(angle),v(axis) {};

	// 四元数 单乘
	void operator *= (float op) {	w*=op; v*=op;	}

	// 四元数相当对比
	//bool operator==(const Quat &q) const { return IsEqual(q,0.0000001f); }
	//bool operator!=(const Quat &q) const { return !(*this == q); }

	 // 四元数求逆
	Quat operator ! () const;

	// 检查是否为单位四元数
	bool IsIdentity() const { return w == 1 && v.x == 0 && v.y == 0 && v.z == 0; }
	void SetIdentity(void);
	static Quat CreateIdentity(void);

	 void SetRotationX( float r );
	static Quat CreateRotationX( float r );
	 void SetRotationY( float r );
	static Quat CreateRotationY( float r );
	 void SetRotationZ( float r );
	static Quat CreateRotationZ( float r );

	float3 GetColumn0() const {return float3(2*(v.x*v.x+w*w)-1,2*(v.y*v.x+v.z*w),2*(v.z*v.x-v.y*w));}
	float3 GetColumn1() const {return float3(2*(v.x*v.y-v.z*w),2*(v.y*v.y+w*w)-1,2*(v.z*v.y+v.x*w));}
	float3 GetColumn2() const {return float3(2*(v.x*v.z+v.y*w),2*(v.y*v.z-v.x*w),2*(v.z*v.z+w*w)-1);}
	float3 GetRow0() const {return float3(2*(v.x*v.x+w*w)-1,2*(v.x*v.y-v.z*w),2*(v.x*v.z+v.y*w));}
	float3 GetRow1() const {return float3(2*(v.y*v.x+v.z*w),2*(v.y*v.y+w*w)-1,2*(v.y*v.z-v.x*w));}
	float3 GetRow2() const	{return float3(2*(v.z*v.x-v.y*w),2*(v.z*v.y+v.x*w),2*(v.z*v.z+w*w)-1);}

	 void Invert( void );
	 Quat GetInverted() const;

	 void	Normalize(void);
	 Quat GetNormalized() const;

	 float GetLength() const;

// 	 bool IsEqual( const Quat& q, float e= SR_EQUAL_PRECISION) const {
// 		Quat p=-q;
// 		bool t0= (abs(v.x-q.v.x)<=e) && (abs(v.y-q.v.y)<=e) && (abs(v.z-q.v.z)<=e) && (abs(w-q.w)<=e);	
// 		bool t1= (abs(v.x-p.v.x)<=e) && (abs(v.y-p.v.y)<=e) && (abs(v.z-p.v.z)<=e) && (abs(w-p.w)<=e);	
// 		t0 |= t1;
// 		return t0;
// 	}

	 bool IsUnit(float e = SR_EQUAL_PRECISION) const
	 {
		 return abs( 1 - ((*this) | (*this)) ) < e;
	 }

	 bool IsValid(float e = SR_EQUAL_PRECISION) const
	 {
		 if (!IsUnit(e))	return false;
		 return true;
	 }
};

inline float operator | (const Quat& q, const Quat& p)
{
	return (q.v.x*p.v.x + q.v.y*p.v.y + q.v.z*p.v.z + q.w*p.w); 
} 

inline Quat  operator * (const Quat &q, const Quat &p) { 
	return Quat
		( 
		q.w*p.w  - (q.v.x*p.v.x + q.v.y*p.v.y + q.v.z*p.v.z),
		q.v.y*p.v.z-q.v.z*p.v.y + q.w*p.v.x + q.v.x*p.w,
		q.v.z*p.v.x-q.v.x*p.v.z + q.w*p.v.y + q.v.y*p.w,
		q.v.x*p.v.y-q.v.y*p.v.x + q.w*p.v.z + q.v.z*p.w  
		);
}

inline void operator *= (Quat &q, const Quat &p) { 
	float s0=q.w; q.w=q.w*p.w-(float3::dot(q.v,p.v)); q.v=p.v*s0+q.v*p.w+(q.v%p.v); 
}

inline  Quat operator / (const Quat &q, const Quat &p) { 
	return (!p*q); 
}
inline  void operator /= (Quat &q, const Quat &p) { 
	q=(!p*q);
}

inline  Quat operator + (const Quat &q, const Quat &p) { 
	return Quat( q.w+p.w, q.v+p.v ); 
}

inline  void operator += (Quat &q, const Quat &p) { 
	q.w+=p.w; q.v+=p.v; 
}

//   Quat operator % (const Quat &q, const Quat &tp) { 
// 	Quat p=tp;
// 	if( (p|q) < 0 ) { p=-p;	} 
// 	return Quat( q.w+p.w, q.v+p.v ); 
// }
// 
//   void operator %= (Quat &q, const Quat &tp) { 
// 	Quat p=tp;
// 	if( (p|q) < 0 ) { p=-p;	} 
// 	q = Quat( q.w+p.w, q.v+p.v ); 
// }

inline  Quat operator - (const Quat &q, const Quat &p) { 
	return Quat( q.w-p.w, q.v-p.v); 
}

inline void operator -= (Quat &q, const Quat &p) { 
	q.w-=p.w; q.v-=p.v; 
}

inline Quat	operator * ( float t, const Quat &q ) {
	return Quat( t*q.w, q.v * t );
};

inline Quat	operator * ( const Quat &q, float t ) {
	return Quat( q.w*t, q.v*t );
};
inline Quat	operator / ( const Quat &q, float t ) {
	return Quat( q.w/t, q.v/t );
};

inline float3 operator * (const Quat &q, const float3 &v) {
	float3 out,r2;
	r2.x=(q.v.y*v.z-q.v.z*v.y)+q.w*v.x;
	r2.y=(q.v.z*v.x-q.v.x*v.z)+q.w*v.y;
	r2.z=(q.v.x*v.y-q.v.y*v.x)+q.w*v.z;
	out.x=(r2.z*q.v.y-r2.y*q.v.z); out.x+=out.x+v.x;
	out.y=(r2.x*q.v.z-r2.z*q.v.x); out.y+=out.y+v.y;
	out.z=(r2.y*q.v.x-r2.x*q.v.y); out.z+=out.z+v.z;
	return out;
}

inline float3 operator * (const float3 &v, const Quat &q) {
	float3 out,r2;
	r2.x=(q.v.z*v.y-q.v.y*v.z)+q.w*v.x;
	r2.y=(q.v.x*v.z-q.v.z*v.x)+q.w*v.y;
	r2.z=(q.v.y*v.x-q.v.x*v.y)+q.w*v.z;
	out.x=(r2.y*q.v.z-r2.z*q.v.y); out.x+=out.x+v.x;
	out.y=(r2.z*q.v.x-r2.x*q.v.z); out.y+=out.y+v.y;
	out.z=(r2.x*q.v.y-r2.y*q.v.x); out.z+=out.z+v.z;
	return out;
}

inline  Quat Quat::operator ! () const { return Quat(w,-v); }
inline  void Quat::Invert( void ) { *this=!*this;	}
inline  Quat Quat::GetInverted() const { return !(*this); } 

inline  Quat Quat::CreateIdentity(void)	{ return Quat(1,0,0,0); }
inline  void	Quat::SetIdentity(void) { w=1; v.x=0; v.y=0; v.z=0; }

inline  Quat Quat::CreateRotationX( float r ) 
{	
	Quat q;	q.SetRotationX(r); 	return q;	
}
inline  void Quat::SetRotationX( float r ) 
{ 
	float s,c; 
	sincosf(float(r*(float)0.5),&s,&c); w=c; v.x=s; v.y=0; v.z=0;	
}

inline  Quat Quat::CreateRotationY( float r ) 
{	
	Quat q;	q.SetRotationY(r); 	return q;	
}
inline  void Quat::SetRotationY( float r ) 
{ 
	float s,c; sincosf(float(r*(float)0.5),&s,&c); w=c; v.x=0; v.y=s; v.z=0;	
}

inline  Quat Quat::CreateRotationZ( float r ) 
{	
	Quat q;	q.SetRotationZ(r); 	return q;	
}
inline  void Quat::SetRotationZ( float r ) 
{ 
	float s,c; sincosf(float(r*(float)0.5),&s,&c); w=c; v.x=0; v.y=0; v.z=s;	
}

inline  void	Quat::Normalize(void)	
{
	float d = isqrtf(w*w + v.x*v.x+v.y*v.y+v.z*v.z);
	w*=d;	v.x*=d; v.y*=d; v.z*=d;
}
inline Quat Quat::GetNormalized() const
{
	Quat t=*this; 
	t.Normalize(); 
	return t;	
}

inline float Quat::GetLength() const 
{ 
	return sqrtf(w*w + v.x*v.x+v.y*v.y+v.z*v.z); 
}

#endif