/**
  @file SrShader.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrShader_h__
#define SrShader_h__

#include "prerequisite.h"
#include "SrResource.h"

SR_ALIGN struct SrShaderContext
{
	float4		  shaderConstants[eSC_ShaderConstantCount];
	SrMatrixArray matrixs;
	SrBitmapArray textureStage;
	SrLightList	  lightList;	
	bool		  alphaTest;
	bool		  culling;

	const void* GetPixelShaderConstantPtr() const
	{
		return &(shaderConstants[eSC_PS0]);
	}

	uint32 Tex2D(float2& texcoord, uint32 stage) const
	{
		uint32 ret = 0x00000000;
		if (stage < textureStage.size() && textureStage[stage] != 0)
		{
			ret = gEnv->renderer->Tex2D(texcoord, textureStage[stage]);
		}
		return ret;
	}

	void * operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void *memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}
};

class SrShader : public SrResource
{
public:
	SrShader(const char* name, ESrVertDecl decl) : SrResource(name, eRT_Shader), m_vertDecl(decl) {}
	virtual ~SrShader() {}

	ESrVertDecl m_vertDecl;
};

#endif // SrShader_h__