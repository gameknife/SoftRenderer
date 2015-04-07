/**
  @file SrCamera.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrCamera_h__
#define SrCamera_h__

SR_ALIGN struct SrCamera
{
	SrCamera( const float3& p, const Quat& r, float fov, float zN, float zF );
	~SrCamera( );

	float44 getViewProjMatrix();
	float44 getViewMatrix();

	void Update();
	void setPos(const float3& pos);
	void setRot(const Quat& rot);
	void setFov(float fov);
	void Move( const float3& localTranslate);
	void Rotate( float yaw, float roll );
	void MarkDirty();

	float getZnear() {return m_zNear;}
	float getZfar() {return m_zFar;}

	void * operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void *memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}
	
	float3 m_farClipVertex[4];
private:
	float3 m_pos;
	Quat m_rot;
	float m_fov;
	float m_zNear;
	float m_zFar;

	float44 cachedView;
	float44 cachedProj;
	float44 cachedViewProj;

	

	bool dirt;
};




#endif
