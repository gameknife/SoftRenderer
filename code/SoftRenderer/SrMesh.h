/**
  @file SrMesh.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrMesh_h__
#define SrMesh_h__

#include "prerequisite.h"
#include "SrResource.h"

class SrEntity;
class SrHairEntity;
SR_ALIGN class SrMesh : public SrResource
{
	friend SrEntity;
	friend SrHairEntity;

public:
	SrMesh(const char* filename);
	virtual ~SrMesh(void);

	void Draw();

	int getSubsetCount() const {return m_subsetCount;}

	void setMaterial(SrMaterial& mat, uint32 index);

	void Destroy();

	void * operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void *memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}

private:
	SrMesh(void);
	void ParseObjFile();
	
	SrPrimitives m_primitives;

	int m_subsetCount;
};

#endif

