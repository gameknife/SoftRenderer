/**
  @file shading.h

  @brief shader基础结构
  
  @author yikaiming

  ver:1.0

 */

#ifndef srswshader_h_
#define srswshader_h_

#include "SrShader.h"

/**
 *@brief ShadingMode
 *@remark 
 */
class SrSwShader : public SrResource
{
public:
	SrSwShader(const char* name):SrResource(name, eRT_Shader) {}
	virtual ~SrSwShader(void) {}

	/**
	 *@brief Patch Shader, 简单的面片渲染
	 *@return void 
	 *@param void * vOut 
	 *@param void * vOut1 
	 *@param void * vOut2 
	 *@param const void * vInRef0 
	 *@param const void * vInRef1 
	 *@param const void * vInRef2 
	 *@param const SrShaderContext * context 
	 */
	virtual void SRFASTCALL ProcessPatch(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const{}
	virtual void SRFASTCALL ProcessVertex(void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context) const =0;
	virtual void SRFASTCALL ProcessRasterize(void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final = false) const = 0;
	virtual void SRFASTCALL ProcessPixel(uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address) const =0;

public:
	class SrShader* m_bindShader;
};

/**
 *@brief 静态接口，计算切空间
 *@return float4 
 *@param const float3 & position1 
 *@param const float3 & position2
 *@param const float3 & position3
 *@param float2 uv1
 *@param float2 uv2 ..
 *@param float2 uv3 ..
 */
static inline float4 CalculateTangentSpaceVector(const float3& position1, const float3& position2, const float3& position3,
	float2& uv1, float2& uv2, float2& uv3)
{
	float3 side0 = position1 - position2;
	float3 side1 = position3 - position1;

	float3 normal = side1 % side0;
	normal.normalize();

	float deltaV0 = uv1.y - uv2.y;
	float deltaV1 = uv3.y - uv1.y;
	float3 tangent = side0 * deltaV1- side1 * deltaV0;
	tangent.normalize();

	float deltaU0 = uv1.x - uv2.x;
	float deltaU1 = uv3.x - uv1.x;
	float3 binormal = side0 * deltaU1 - side1 * deltaU0;
	binormal.normalize();

	float3 tangentCross = tangent % binormal;
	float flip = 1.f;
	if ( float3::dot(tangentCross,normal) < 0.0f)
	{
		tangent = -tangent;
		binormal = -binormal;
		flip = -1.f;
	}

	return float4( tangent, flip);
}

/**
 *@brief Blinn-Phong光照模型
 *@return void 
 *@param float3 normalDir 
 *@param float3 lightDir 
 *@param float3 viewDir 
 *@param float power 
 *@param float & diffuse 
 *@param float & specular 
 */
static inline void BlinnBRDF( float3& normalDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL, 0.f, 1.f );

	// blinn BDRF
	float3 H = lightDir + viewDir;
	H.normalize();
	specular = pow( Clamp( float3::dot(H, normalDir),0.f, 1.f), power ) * diffuse;
}

/**
 *@brief Phongu光照模型
 *@return void 
 *@param float3 normalDir
 *@param float3 lightDir
 *@param float3 viewDir
 *@param float power
 *@param float & diffuse
 *@param float & specular
 */
static inline void PhongBRDF( float3& normalDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL, 0.f, 1.f );

	// phong BDRF
	float3 reflectWS = viewDir;
	reflectWS.reflect( -viewDir, normalDir );
	specular = pow( Clamp( float3::dot(reflectWS, lightDir),0.f, 1.f), 15.f ) * diffuse;
}

/**
 *@brief Fresnel通用计算
 *@return float 
 *@param float NdotE 
 *@param float bias 
 *@param float power 
 *@param float scale 
 */
static inline float GetFresnel(float NdotE, float bias, float power, float scale)
{
	return bias + scale * pow(1.f - NdotE, power);
}

/**
 *@brief nv的快速皮肤模拟
 *@return void 
 *@param float NdotL 
 *@param float RollOff 
 *@param float & subSurface 
 */
static inline void nvLambSkin(float NdotL,
	float RollOff,
	float& subSurface
	) 
{
	subSurface = SmoothStep(-RollOff,1.0f,NdotL) - SmoothStep(1.0f,NdotL);
	subSurface = fmax(0.0f,subSurface);
}

/**
 *@brief 光照封装
 *@return void 
 *@param const SrShaderContext * context 
 *@param float3 worldPos
 *@param float3 normalDir
 *@param float3 viewWS
 *@param float4 & diffuseAcc
 *@param float4 & specularAcc
 */
static inline void CalcLights( const SrShaderContext* context, float3& worldPos, float3& normalDir, float3& viewWS, float4 &diffuseAcc, float4 &specularAcc )
{
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	for (uint32 i=0; i < context->lightList.size(); ++i)
	{
		SrLight* lt = context->lightList[i];
		float3 lightPosWS = lt->worldPos;
		float3 lightDirWS = lightPosWS - worldPos;
		lightDirWS.normalize();

		float diffuse;
		float specular;
		BlinnBRDF(normalDir, lightDirWS, viewWS, cBuffer->glossness, diffuse, specular);

		diffuseAcc += (lt->diffuseColor * diffuse);
		specularAcc += (lt->specularColor * specular);
	}
}

/**
 *@brief 皮肤光照封装
 *@return void 
 *@param const SrShaderContext * context 
 *@param float3 worldPos 
 *@param float3 normalDir 
 *@param float3 viewWS 
 *@param float4 subSurfaceCol 
 *@param float4 & diffuseAcc 
 *@param float4 & specularAcc 
 */
static inline void CalcLightsSkin( const SrShaderContext* context, float3& worldPos, float3& normalDir, float3& viewWS, float4& subSurfaceCol, float4 &diffuseAcc, float4 &specularAcc )
{
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	for (uint32 i=0; i < context->lightList.size(); ++i)
	{
		SrLight* lt = context->lightList[i];
		float3 lightPosWS = lt->worldPos;
		float3 lightDirWS = lightPosWS - worldPos;
		lightDirWS.normalize();

		float diffuse;
		float specular;
		float subSurface;
		BlinnBRDF(normalDir, lightDirWS, viewWS, cBuffer->glossness, diffuse, specular);
		nvLambSkin( float3::dot( lightDirWS, normalDir ) , 0.5f, subSurface);

		diffuseAcc += ((lt->diffuseColor * diffuse) + subSurfaceCol * subSurface);
		specularAcc += (lt->specularColor * specular);
	}
}

#endif // shading_h__