/**
  @file SrCustomShader.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef AdvanceShaders_h__
#define AdvanceShaders_h__

#include "prerequisite.h"
#include "SrSwShader.h"

class SrSkinSimShader : public SrSwShader
{
public:
	SrSkinSimShader():SrSwShader("skin") {}
	~SrSkinSimShader() {}

	virtual void SRFASTCALL ProcessPatch(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
};

extern SrSkinSimShader g_SkinSimShader;

class SrFresnelNormalShader : public SrSwShader
{
public:
	SrFresnelNormalShader():SrSwShader("fresnel") {}
	~SrFresnelNormalShader() {}

	virtual void SRFASTCALL ProcessPatch(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
};

extern SrFresnelNormalShader g_FresnelNormalShader;

void LoadCustomShaders();

#endif // SrCustomShader_h__