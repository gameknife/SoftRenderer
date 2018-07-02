/**
  @file shading.h

  @brief ��ɫ��ͼ���㷨
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef srswshader_h_
#define srswshader_h_

#include "RendererBase.h"

#include "SrShader.h"

/**
 *@brief ShadingMode����
 *@remark Shader�࣬�����������ɫ���ԡ�����VS,RS,PSÿ����ɫ����ʵ��
   PATCH SHADERΪ��դ��ǰ�Զ���������Ƭ�Ĵ�����ᣬ��Ҫ������һ�׶ε���ɫ���Կ���ѡ��ʵ�֡�
 */
class SrSwShader : public SrResource
{
public:
	SrSwShader(const char* name):SrResource(name, eRT_Shader) {}
	virtual ~SrSwShader(void) {}

	/**
	 *@brief Patch Shader, �ڹ�դ��ǰ��������������������������������ɫ�����̳е���ɫ���԰���ʵ�֡�
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
 *@brief ���ߵļ��㺯��
 *@return float4 ���ص�����
 *@param const float3 & position1 ��������ģ�Ϳռ�λ��
 *@param const float3 & position2 ͬ�������εĵ�һ������
 *@param const float3 & position3 ͬ�������εĵڶ�������
 *@param float2 uv1 ��������uv
 *@param float2 uv2 ..
 *@param float2 uv3 ..
 */
static inline float4 CalculateTangentSpaceVector(const float3& position1, const float3& position2, const float3& position3,
	float2& uv1, float2& uv2, float2& uv3)
{
	// ����������ε�����
	float3 side0 = position1 - position2;
	float3 side1 = position3 - position1;
	// ��˵ó�����
	float3 normal = side1 % side0;
	normal.normalize();
	// �ó�tangent����
	float deltaV0 = uv1.y - uv2.y;
	float deltaV1 = uv3.y - uv1.y;
	float3 tangent = side0 * deltaV1- side1 * deltaV0;
	tangent.normalize();
	// �ó�binormal����
	float deltaU0 = uv1.x - uv2.x;
	float deltaU1 = uv3.x - uv1.x;
	float3 binormal = side0 * deltaU1 - side1 * deltaU0;
	binormal.normalize();

	// ���¼���normal������ͳ�ʼnormal�෴����xy��������һ�£���Ҫ����
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
 *@brief Blinn-Phong����ģ��
 *@return void 
 *@param float3 normalDir ���߷���
 *@param float3 lightDir ���շ���
 *@param float3 viewDir ���߷���
 *@param float power �����
 *@param float & diffuse �����diffuse
 *@param float & specular �����specular
 */
static inline void BlinnBRDF( float3& normalDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	// Lambert������
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL, 0.f, 1.f );

	// blinn BDRF
	float3 H = lightDir + viewDir;
	H.normalize();
	specular = pow( Clamp( float3::dot(H, normalDir),0.f, 1.f), power ) * diffuse;
}

static inline void Kajiya_Kay_BRDF( float3& normalDir, float3& tangentDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	// Kajiya_Kay������
	//float TdotL = float3::dot( tangentDir, lightDir );
	//diffuse = sqrt(1.f - TdotL * TdotL );

	// Half-Lambert������
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL * 0.5f + 0.5f, 0.f, 1.f );

	// blinn BDRF
	float3 H = lightDir + viewDir;
	H.normalize();
	float HdotT = float3::dot(H, tangentDir);
	specular = pow(  sqrt( 1.f - HdotT * HdotT), power ) * diffuse;
}

/**
 *@brief Phong����ģ��
 *@return void 
 *@param float3 normalDir ���߷���
 *@param float3 lightDir ���շ���
 *@param float3 viewDir ���߷���
 *@param float power �����
 *@param float & diffuse �����diffuse
 *@param float & specular �����specular
 */
static inline void PhongBRDF( float3& normalDir, float3& lightDir, float3& viewDir, float power, float& diffuse, float& specular )
{
	// Lambert������
	float NdotL = float3::dot( normalDir, lightDir );
	diffuse = Clamp( NdotL, 0.f, 1.f );

	// phong BDRF
	float3 reflectWS = viewDir;
	reflectWS.reflect( -viewDir, normalDir );
	specular = pow( Clamp( float3::dot(reflectWS, lightDir),0.f, 1.f), 15.f ) * diffuse;
}

/**
 *@brief ���ٷ������㷨
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
 *@brief nv�Ŀ��ٴα�����͸ģ��
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
 *@brief һ���Դ�Ķ��Դ��������
 *@return void 
 *@param const SrShaderContext * context ������
 *@param float3 worldPos Ԫ�ص�����ռ�����
 *@param float3 normalDir Ԫ�ص�����ռ䷨����
 *@param float3 viewWS Ԫ�ص�����ռ�����
 *@param float4 & diffuseAcc ���ص�������ۼ�
 *@param float4 & specularAcc ���صľ��淴��ۼ�
 */
static inline void CalcLights( const SrShaderContext* context, float3& worldPos, float3& normalDir, float3& viewWS, float4 &diffuseAcc, float4 &specularAcc )
{
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	for (uint32 i=0; i < context->lightList.size(); ++i)
	{
		// ���Դ��ȡ�ù��շ���
		SrLight* lt = context->lightList[i];
		float3 lightPosWS = lt->worldPos;
		float3 lightDirWS = lightPosWS - worldPos;
		lightDirWS.normalize();

		// ���ռ���
		float diffuse;
		float specular;
		BlinnBRDF(normalDir, lightDirWS, viewWS, cBuffer->glossness, diffuse, specular);

		// �ۼӵ��ۼ�
		diffuseAcc += (lt->diffuseColor * diffuse);
		specularAcc += (lt->specularColor * specular);
	}


}

static inline void CalcLightsKajiya_Kay( const SrShaderContext* context, float3& worldPos, float3& normalDir, float3& tangentDir, float3& viewWS, float4 &diffuseAcc, float4 &specularAcc )
{
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	for (uint32 i=0; i < context->lightList.size(); ++i)
	{
		// ���Դ��ȡ�ù��շ���
		SrLight* lt = context->lightList[i];
		float3 lightPosWS = lt->worldPos;
		float3 lightDirWS = lightPosWS - worldPos;
		lightDirWS.normalize();

		// ���ռ���
		float diffuse;
		float specular;
		Kajiya_Kay_BRDF(normalDir, tangentDir, lightDirWS, viewWS, cBuffer->glossness, diffuse, specular);

		// �ۼӵ��ۼ�
		diffuseAcc += (lt->diffuseColor * diffuse);
		specularAcc += (lt->specularColor * specular);
	}
}

/**
 *@brief ��Զ��Դ��Ƥ���������
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

		// ���ռ���
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