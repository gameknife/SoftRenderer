/**
  @file SrCustomShader.h
  
  @author yikaiming

  ������־ history
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
	virtual uint32 GetPixelDependencyFlags() const { return ePd_LocalPixel; }
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
	virtual uint32 GetPixelDependencyFlags() const { return ePd_LocalPixel; }
};

extern SrFresnelNormalShader g_FresnelNormalShader;

class SrHairShader : public SrSwShader
{
public:
	SrHairShader():SrSwShader("hair") {}
	~SrHairShader() {}

	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
	virtual uint32 GetPixelDependencyFlags() const { return ePd_LocalPixel; }
};

extern SrHairShader g_HairShader;

void LoadCustomShaders();

#endif // SrCustomShader_h__
