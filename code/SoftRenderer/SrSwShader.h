/**
  @file shading.h

  @brief 着色用图形算法
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef srswshader_h_
#define srswshader_h_

#include "RendererBase.h"

#include "SrShader.h"

/**
 *@brief ShadingMode基类
 *@remark Shader类，涵盖物体的着色策略。其中VS,RS,PS每种着色必须实现
   PATCH SHADER为光栅化前对独立三角面片的处理机会，需要处理这一阶段的着色策略可以选择实现。
 */
class SrSwShader : public SrResource
{
public:
	SrSwShader(const char* name):SrResource(name, eRT_Shader) {}
	virtual ~SrSwShader(void) {}

	/**
	 *@brief Patch Shader, 在光栅化前，对三角面三个顶点进行重新运算的着色器。继承的着色策略按需实现。
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
 *@brief 切线的计算函数
 *@return float4 返回的切线
 *@param const float3 & position1 这个顶点的模型空间位置
 *@param const float3 & position2 同在三角形的第一个顶点
 *@param const float3 & position3 同在三角形的第二个顶点
 *@param float2 uv1 这个顶点的uv
 *@param float2 uv2 ..
 *@param float2 uv3 ..
 */
static inline float4 CalculateTangentSpaceVector(const float3& position1, const float3& position2, const float3& position3,
	float2& uv1, float2& uv2, float2& uv3)
{
	// 计算出三角形的两边
	float3 side0 = position1 - position2;
	float3 side1 = position3 - position1;
	// 插乘得出法线
	float3 normal = side1 % side0;
	normal.normalize();
	// 得出tangent方向
	float deltaV0 = uv1.y - uv2.y;
	float deltaV1 = uv3.y - uv1.y;
	float3 tangent = side0 * deltaV1- side1 * deltaV0;
	tangent.normalize();
	// 得出binormal方向
	float deltaU0 = uv1.x - uv2.x;
	float deltaU1 = uv3.x - uv1.x;
	float3 binormal = side0 * deltaU1 - side1 * deltaU0;
	binormal.normalize();

	// 重新计算normal，如果和初始normal相反，则xy增长方向不一致，需要纠正
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
 *@param float3 normalDir 法线方向
 *@param float3 lightDir 光照方向
 *@param float3 viewDir 视线方向
 *@param float power 光泽度
 *@param float & diffuse 输出的diffuse
 *@param float & specular 输出的specular
 */
static inline void BlinnBRDF( float3& normalDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	// Lambert漫反射
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL, 0.f, 1.f );

	// blinn BDRF
	float3 H = lightDir + viewDir;
	H.normalize();
	specular = pow( Clamp( float3::dot(H, normalDir),0.f, 1.f), power ) * diffuse;
}

static inline void Kajiya_Kay_BRDF( float3& normalDir, float3& tangentDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	// Kajiya_Kay漫反射
	//float TdotL = float3::dot( tangentDir, lightDir );
	//diffuse = sqrt(1.f - TdotL * TdotL );

	// Half-Lambert漫反射
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL * 0.5f + 0.5f, 0.f, 1.f );

	// blinn BDRF
	float3 H = lightDir + viewDir;
	H.normalize();
	float HdotT = float3::dot(H, tangentDir);
	specular = pow(  sqrt( 1.f - HdotT * HdotT), power ) * diffuse;
}

/**
 *@brief Phong光照模型
 *@return void 
 *@param float3 normalDir 法线方向
 *@param float3 lightDir 光照方向
 *@param float3 viewDir 视线方向
 *@param float power 光泽度
 *@param float & diffuse 输出的diffuse
 *@param float & specular 输出的specular
 */
static inline void PhongBRDF( float3& normalDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	// Lambert漫反射
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL, 0.f, 1.f );

	// phong BDRF
	float3 reflectWS = viewDir;
	reflectWS.reflect( -viewDir, normalDir );
	specular = pow( Clamp( float3::dot(reflectWS, lightDir),0.f, 1.f), 15.f ) * diffuse;
}

/**
 *@brief 快速菲涅尔算法
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
 *@brief nv的快速次表面渗透模拟
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
	subSurface = max(0.0f,subSurface);
}


/**
 *@brief 一般光源的多光源光照运算
 *@return void 
 *@param const SrShaderContext * context 上下文
 *@param float3 worldPos 元素的世界空间坐标
 *@param float3 normalDir 元素的世界空间法向量
 *@param float3 viewWS 元素的世界空间视线
 *@param float4 & diffuseAcc 返回的漫反射聚集
 *@param float4 & specularAcc 返回的镜面反射聚集
 */
static inline void CalcLights( const SrShaderContext* context, float3& worldPos, float3& normalDir, float3& viewWS, float4 &diffuseAcc, float4 &specularAcc )
{
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	for (uint32 i=0; i < context->lightList.size(); ++i)
	{
		// 点光源，取得光照方向
		SrLight* lt = context->lightList[i];
		float3 lightPosWS = lt->worldPos;
		float3 lightDirWS = lightPosWS - worldPos;
		lightDirWS.normalize();

		// 光照计算
		float diffuse;
		float specular;
		BlinnBRDF(normalDir, lightDirWS, viewWS, cBuffer->glossness, diffuse, specular);

		// 累加到聚集
		diffuseAcc += (lt->diffuseColor * diffuse);
		specularAcc += (lt->specularColor * specular);
	}


}

static inline void CalcLightsKajiya_Kay( const SrShaderContext* context, float3& worldPos, float3& normalDir, float3& tangentDir, float3& viewWS, float4 &diffuseAcc, float4 &specularAcc )
{
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	for (uint32 i=0; i < context->lightList.size(); ++i)
	{
		// 点光源，取得光照方向
		SrLight* lt = context->lightList[i];
		float3 lightPosWS = lt->worldPos;
		float3 lightDirWS = lightPosWS - worldPos;
		lightDirWS.normalize();

		// 光照计算
		float diffuse;
		float specular;
		Kajiya_Kay_BRDF(normalDir, tangentDir, lightDirWS, viewWS, cBuffer->glossness, diffuse, specular);

		// 累加到聚集
		diffuseAcc += (lt->diffuseColor * diffuse);
		specularAcc += (lt->specularColor * specular);
	}
}

/**
 *@brief 针对多光源的皮肤光照运算
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

		// 光照计算
		float diffuse;
		float specular;
		float subSurface;
		BlinnBRDF(normalDir, lightDirWS, viewWS, cBuffer->glossness, diffuse, specular);
		nvLambSkin( float3::dot( lightDirWS, normalDir ) , 0.5f, subSurface);

		// 
		diffuseAcc += ((lt->diffuseColor * diffuse) + subSurfaceCol * subSurface);
		specularAcc += (lt->specularColor * specular);
	}
}

#endif // shading_h__