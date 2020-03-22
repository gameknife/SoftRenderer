/**
  @file SrShadingMode.h
  
  @author yikaiming

  @breif 着色模式抽象基类以及基础flat, gourand, phong着色模式的实现类

  更改日志 history
  ver:1.0
   
 */

#ifndef BasicShaders_h__
#define BasicShaders_h__

#include "prerequisite.h"
#include "SrSwShader.h"


class SrFlatShader : public SrSwShader
{
public:
	SrFlatShader():SrSwShader("flat") { m_maxChannel = 1; }
	~SrFlatShader() {}

	virtual void SRFASTCALL ProcessPatch(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
};

class SrGourandShader : public SrSwShader
{
public:
	SrGourandShader():SrSwShader("gourand") { m_maxChannel = 1; }
	~SrGourandShader() {}

	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
};

class SrPhongShader : public SrSwShader
{
public:
	SrPhongShader() :SrSwShader("default") { m_maxChannel = 3; }
	~SrPhongShader() {}

	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context , uint32 address) const;
};


class SrPhongWithNormalShader : public SrSwShader
{
public:
	SrPhongWithNormalShader():SrSwShader("default_normal") { m_maxChannel = 4; }
	~SrPhongWithNormalShader() {}

	virtual void SRFASTCALL ProcessPatch(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const;
	virtual void SRFASTCALL ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false ) const;
	virtual void SRFASTCALL ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const;
};


// GLOBAL SHADER
extern SrPhongShader g_PhongShadingShader;
extern SrFlatShader g_FlatShadingShader;
extern SrGourandShader g_GourandShadingShader;
extern SrPhongWithNormalShader g_PhongShadingWithNormalShader;
void LoadInternalShaders();
#endif // SrShadingMode_h__


