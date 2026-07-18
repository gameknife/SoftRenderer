/**
  @file SrShadingMode.h
  
  @author yikaiming

  @breif ��ɫģʽ��������Լ�����flat, gourand, phong��ɫģʽ��ʵ����

  ������־ history
  ver:1.0
   
 */

#ifndef BasicShaders_h__
#define BasicShaders_h__

#include "prerequisite.h"
#include "SrSwShader.h"


class SrFlatShader : public SrSwShader
{
public:
	SrFlatShader():SrSwShader("flat") {}
	~SrFlatShader() {}

	virtual void SRFASTCALL ProcessPatch(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
	virtual uint32 GetPixelDependencyFlags() const { return ePd_LocalPixel; }
};

class SrGourandShader : public SrSwShader
{
public:
	SrGourandShader():SrSwShader("gourand") {}
	~SrGourandShader() {}

	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
	virtual uint32 GetPixelDependencyFlags() const { return ePd_LocalPixel; }
};

class SrPhongShader : public SrSwShader
{
public:
	SrPhongShader():SrSwShader("default") {}
	~SrPhongShader() {}

	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context , uint32 address) const;
	virtual uint32 GetPixelDependencyFlags() const { return ePd_LocalPixel; }
};


class SrPhongWithNormalShader : public SrSwShader
{
public:
	SrPhongWithNormalShader():SrSwShader("default_normal") {}
	~SrPhongWithNormalShader() {}

	virtual void SRFASTCALL ProcessPatch(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
	virtual uint32 GetPixelDependencyFlags() const { return ePd_ReadsFullGBuffer; }
};


// GLOBAL SHADER
extern SrPhongShader g_PhongShadingShader;
extern SrFlatShader g_FlatShadingShader;
extern SrGourandShader g_GourandShadingShader;
extern SrPhongWithNormalShader g_PhongShadingWithNormalShader;
void LoadInternalShaders();
#endif // SrShadingMode_h__


