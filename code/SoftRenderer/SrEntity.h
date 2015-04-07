/**
  @file SrEntity.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrEntity_h__
#define SrEntity_h__

class SrMesh;

SR_ALIGN class SrEntity
{
public:
	SrEntity(void);
	virtual ~SrEntity(void);

	float44 getWorldMatrix();
	
	void SetPos( float3& pos );
	void SetRotation( Quat& rot );

	void MoveLocal( float3& trans );
	void RotateLocal( float3& angle );

	void SetScale( float3& scale);

	void Update();
	virtual void Update(float fElapsedTime) {}

	virtual void CreateMesh( const char* filename );
	void LoadMaterial( const char* filename );

	virtual void Draw();

	void SetVisible(bool visible) {m_visible = visible;}
	bool IsVisible() {return m_visible;}

	SrMaterial* getMaterial(uint32 index = 0);
	uint32 getMaterialCount();

	void * operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void *memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}

protected:

	float3 m_pos;
	Quat m_rot;
	float3 m_scale;
	
	float44 m_worldMatrix;

	bool m_dirty;

	SrMesh*		m_mesh;

	bool m_visible;
};

#endif

