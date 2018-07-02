/**
  @file SrCustomShader.cpp
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#include "stdafx.h"
#include "AdvanceShaders.h"

#include "mmgr.h"

SrSkinSimShader g_SkinSimShader;
SrFresnelNormalShader g_FresnelNormalShader;
SrHairShader g_HairShader;
// local data format
struct SrFresnelNormal_Vert2Frag
{
	float4 pos;				// channel0

	float4 worldpos_tx;		// channel1:xyz
	float4 normal_ty;			// channel1:w channel2:xy
	float4 tangent;			// channel2:zw channel3:x
	// full, cannot add any more
};

void SrSkinSimShader::ProcessPatch( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const
{
	SrFresnelNormal_Vert2Frag* inTHREE[3] = {(SrFresnelNormal_Vert2Frag*)vInRef0, (SrFresnelNormal_Vert2Frag*)vInRef1, (SrFresnelNormal_Vert2Frag*)vInRef2};
	SrFresnelNormal_Vert2Frag* outTHREE[3] = {(SrFresnelNormal_Vert2Frag*)vOut, (SrFresnelNormal_Vert2Frag*)vOut1, (SrFresnelNormal_Vert2Frag*)vOut2};

	//tangentProcess
	// calc tangent space in vertex shader!
	float3 pos1 = inTHREE[0]->tangent.xyz;
	float3 pos2 = inTHREE[1]->tangent.xyz;
	float3 pos3 = inTHREE[2]->tangent.xyz;

	float2 uv1(inTHREE[0]->worldpos_tx.w, inTHREE[0]->normal_ty.w);
	float2 uv2(inTHREE[1]->worldpos_tx.w, inTHREE[1]->normal_ty.w);
	float2 uv3(inTHREE[2]->worldpos_tx.w, inTHREE[2]->normal_ty.w);

	outTHREE[0]->tangent = CalculateTangentSpaceVector(pos1, pos2, pos3, uv1, uv2, uv3);
	outTHREE[1]->tangent = CalculateTangentSpaceVector(pos2, pos1, pos3, uv2, uv1, uv3);
	outTHREE[2]->tangent = CalculateTangentSpaceVector(pos3, pos2, pos1, uv3, uv2, uv1);

	outTHREE[0]->tangent.xyz = (context->matrixs[eMd_World].RotateVector3(outTHREE[0]->tangent.xyz ));
	outTHREE[1]->tangent.xyz = (context->matrixs[eMd_World].RotateVector3(outTHREE[1]->tangent.xyz ));
	outTHREE[2]->tangent.xyz = (context->matrixs[eMd_World].RotateVector3(outTHREE[2]->tangent.xyz ));
}

void SrSkinSimShader::ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const
{
	SrVertexP3N3T2* in = (SrVertexP3N3T2*)vInRef0;
	SrFresnelNormal_Vert2Frag* out = (SrFresnelNormal_Vert2Frag*)vOut;


	// pos����ͶӰ�ռ�
	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;
	// pos��������ռ䣬����
	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;
	// normal,tangent��������ռ�
	out->normal_ty = float4( (context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);

	// ��ʱ��pos����tangent����geometry�׶μ���tangent
	out->tangent.xyz = in->pos.xyz;// * out->pos.w;
	//out->texcoord2 = in->texcoord * out->pos.w;
}

void SrSkinSimShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	// ���Բ�ֵproject space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// �Ѿ���w
	// ֱ�Ӳ�ֵ������channel
	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

	// ����scanlineɨ��ģ���͸�Ӳ�ֵ���꣬��ֵ������ֵ
	if (final)
	{
		verO->normal_ty /= verO->pos.w;
		verO->worldpos_tx /= verO->pos.w;
		verO->tangent /= verO->pos.w;
	}
}

/**
 *@brief Skinģ��Pixel Shader
 *@return void 
 *@param uint32 * pOut 
 *@param const void * pIn 
 *@param const SrShaderContext * context 
 *@remark ʹ��nvLamb������NdotL��ȡ�α���ɢ���ģ�⴩͸ֵ���Ӵα�����ɫͼ�ϲ�����Ӧ����ɫ����Ϊ���յĲ��䡣
 ͬʱ������NdotE����ȡ��Ե��ǿ�ȣ����Ի�����ɫ�Ա���Ƥ���ͨ͸�С�
 */
void SrSkinSimShader::ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const
{
	SrFresnelNormal_Vert2Frag* in = (SrFresnelNormal_Vert2Frag*)pIn;
	uint32* out = (uint32*)pOut;
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	// ����diffuse��ɫ
	float2 tc0(in->worldpos_tx.w, in->normal_ty.w);

	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	// �����α�����ɫ
	col = context->Tex2D( tc0, 4 );
	float4 matSSS = uint32_2_float4(col);

	// diffuse��ɫ �� srgb
	matDiff = matDiff * matDiff;

	// �����Ŷ�����
	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();
	float3 tangentDir = in->tangent.xyz;
	tangentDir.normalize();
	float3 binormalDir = tangentDir % normalDir;

	// ��������ռ�� ���ߣ����ߣ������� ��������߿ռ䵽����ռ��ת������
	float33 tangent2world( tangentDir, binormalDir, normalDir );

	// ����normal
	uint32 nor = context->Tex2D( tc0, 1 );
	float4 norf = uint32_2_float4(nor);

	// ��0-1����-1��1�ռ�
	float3 normalTangent = norf.xyz;
	normalTangent = (normalTangent - float3(0.5f)) * 2.f;

	// �������������߿ռ䷨��ת��������ռ�
	normalDir = tangent2world * normalTangent;
	normalDir.normalize();
	
	// ���߷������
	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	// ���߷��䷽��
	float3 viewReflWS;
	viewReflWS.reflect(viewWS, normalDir);

	// ������������
	float2 calcTexcoord = viewReflWS.xy * 0.5f + float2(0.5f, 0.5f);
	uint32 refl = context->Tex2D( calcTexcoord, 3 );
	float4 reflf = uint32_2_float4(refl);

	// ȡ��ȫ����⣬������normal���������ģ��
	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc(0.f);

	// ����shading lib��Ƥ����ռ��㺯��������diffuse��specular�Ĺ����ۻ�
	CalcLightsSkin(context, in->worldpos_tx.xyz, normalDir, viewWS, matSSS, diffuseAcc, specularAcc);

	
	float NdotE = float3::dot(viewWS, normalDir);
	//float fresnel = GetFresnel(NdotE, cBuffer->fresnelBia, cBuffer->fresnelPower, cBuffer->fresnelScale);
	float rim = GetFresnel(NdotE, 0.01f, 5.f, 20.f);

	// ���շ���1�����ʹ���ɫ * ������ͻ����Ĺ��վۼ�
	specularAcc += (reflf * diffuseAcc) * rim;

	// rim light
	//specularAcc += ( float4( 1.f, 1.f, 0.95686f, 0.847f ) * diffuseAcc ) * rim;

	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;
	// ���շ���2�����ʷ���ɫ * ���淴����վۼ�
	diffuseAcc += (cBuffer->spcColor * matDiff.a) * (specularAcc) ;

	// ����ɫ�ʽضϵ�0-1
	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	// srgb��ԭ
	diffuseAcc.sqrt();

	// float4 �� uint32�����
	*out = float4_2_uint32(diffuseAcc);
}

void SrFresnelNormalShader::ProcessPatch( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const
{
	SrFresnelNormal_Vert2Frag* inTHREE[3] = {(SrFresnelNormal_Vert2Frag*)vInRef0, (SrFresnelNormal_Vert2Frag*)vInRef1, (SrFresnelNormal_Vert2Frag*)vInRef2};
	SrFresnelNormal_Vert2Frag* outTHREE[3] = {(SrFresnelNormal_Vert2Frag*)vOut, (SrFresnelNormal_Vert2Frag*)vOut1, (SrFresnelNormal_Vert2Frag*)vOut2};

	//tangentProcess
	// calc tangent space in vertex shader!
	float3 pos1 = inTHREE[0]->tangent.xyz;
	float3 pos2 = inTHREE[1]->tangent.xyz;
	float3 pos3 = inTHREE[2]->tangent.xyz;

	float2 uv1(inTHREE[0]->worldpos_tx.w, inTHREE[0]->normal_ty.w);
	float2 uv2(inTHREE[1]->worldpos_tx.w, inTHREE[1]->normal_ty.w);
	float2 uv3(inTHREE[2]->worldpos_tx.w, inTHREE[2]->normal_ty.w);

	outTHREE[0]->tangent = CalculateTangentSpaceVector(pos1, pos2, pos3, uv1, uv2, uv3);
	outTHREE[1]->tangent = CalculateTangentSpaceVector(pos2, pos1, pos3, uv2, uv1, uv3);
	outTHREE[2]->tangent = CalculateTangentSpaceVector(pos3, pos2, pos1, uv3, uv2, uv1);

	outTHREE[0]->tangent.xyz = (context->matrixs[eMd_World].RotateVector3(outTHREE[0]->tangent.xyz ));
	outTHREE[1]->tangent.xyz = (context->matrixs[eMd_World].RotateVector3(outTHREE[1]->tangent.xyz ));
	outTHREE[2]->tangent.xyz = (context->matrixs[eMd_World].RotateVector3(outTHREE[2]->tangent.xyz ));
}

void SrFresnelNormalShader::ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const
{
	SrVertexP3N3T2* in = (SrVertexP3N3T2*)vInRef0;
	SrFresnelNormal_Vert2Frag* out = (SrFresnelNormal_Vert2Frag*)vOut;

	// pos����ͶӰ�ռ�
	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;
	// pos��������ռ䣬����
	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;
	// normal,tangent��������ռ�
	out->normal_ty = float4( (context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);

	// ��ʱ��pos����tangent����geometry�׶μ���tangent
	out->tangent.xyz = in->pos.xyz;// * out->pos.w;
	//out->texcoord2 = in->texcoord * out->pos.w;
}

void SrFresnelNormalShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	// ���Բ�ֵproject space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// �Ѿ���w
	// ֱ�Ӳ�ֵ������channel
	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

	// ����scanlineɨ��ģ���͸�Ӳ�ֵ���꣬��ֵ������ֵ
	if (final)
	{
		verO->normal_ty /= verO->pos.w;
		verO->worldpos_tx /= verO->pos.w;
		verO->tangent /= verO->pos.w;
	}
}

void SrFresnelNormalShader::ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context , uint32 address) const
{
	SrFresnelNormal_Vert2Frag* in = (SrFresnelNormal_Vert2Frag*)pIn;
	uint32* out = (uint32*)pOut;
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	// ����diffuse��ɫ
	float2 tc0(in->worldpos_tx.w, in->normal_ty.w);
	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	// diffuse �� srgb
	matDiff = matDiff * matDiff;

	// �����Ŷ�����
	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();
	float3 tangentDir = in->tangent.xyz;
	tangentDir.normalize();
	float3 binormalDir = tangentDir % normalDir;

	// ��������ռ�� ���ߣ����ߣ������� ��������߿ռ䵽����ռ��ת������
	float33 tangent2world( tangentDir, binormalDir, normalDir );

	// ����normal
	uint32 nor = context->Tex2D( tc0, 1 );
	float4 norf = uint32_2_float4(nor);

	float3 normalTangent = norf.xyz;
	normalTangent = (normalTangent - float3(0.5f)) * 2.f;
	//normalTangent.y *= -1;

	// �������������߿ռ䷨��ת��������ռ�
	normalDir = tangent2world * normalTangent;
	normalDir.normalize();

	// ���߷������
	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	// ���߷��䷽��
	float3 viewReflWS;
	viewReflWS.reflect(viewWS, normalDir);

	// ������������
	float2 calcTexcoord = viewReflWS.xy * 0.5f + float2(0.5f, 0.5f);
	uint32 refl = context->Tex2D( calcTexcoord, 3 );
	float4 reflf = uint32_2_float4(refl) * 0.25f;


	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc(0.f);

	CalcLights(context, in->worldpos_tx.xyz, normalDir, viewWS, diffuseAcc, specularAcc);

	float NdotE = float3::dot(viewWS, normalDir);
	float fresnel = GetFresnel(NdotE, cBuffer->fresnelBia, cBuffer->fresnelPower, cBuffer->fresnelScale);

	specularAcc += reflf * diffuseAcc;
	// �ضϵ�0-1
	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;
	diffuseAcc += ((cBuffer->spcColor) * specularAcc) * fresnel;
	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	// srgb��ԭ
	diffuseAcc.sqrt();

	*out = float4_2_uint32(diffuseAcc);
}

void SRFASTCALL SrHairShader::ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const
{
	SrVertexP3N3T2* in = (SrVertexP3N3T2*)vInRef0;
	SrFresnelNormal_Vert2Frag* out = (SrFresnelNormal_Vert2Frag*)vOut;

	// pos����ͶӰ�ռ�
	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;
	// pos��������ռ䣬����
	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;
	// normal,tangent��������ռ�
	out->normal_ty = float4((context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);
}

void SRFASTCALL SrHairShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final /*= false */ ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	// ���Բ�ֵproject space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// �Ѿ���w
	// ֱ�Ӳ�ֵ������channel
	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

	// ����scanlineɨ��ģ���͸�Ӳ�ֵ���꣬��ֵ������ֵ
	if (final)
	{
		verO->normal_ty /= verO->pos.w;
		verO->worldpos_tx /= verO->pos.w;
		verO->tangent /= verO->pos.w;
	}
}

void SRFASTCALL SrHairShader::ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const
{
	SrFresnelNormal_Vert2Frag* in = (SrFresnelNormal_Vert2Frag*)pIn;
	uint32* out = (uint32*)pOut;
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	// ����diffuse��ɫ
	float2 tc0( in->worldpos_tx.w, in->normal_ty.w );
	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	// diffuse �� srgb
	matDiff = matDiff * matDiff;

	// �����Ŷ�����
	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();

	// 	*out = float4_2_uint32(float4((normalDir * 0.5f + float3(0.5f)),1.f));
	// 	return;


	// ���߷������
	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc(0.f);

	CalcLightsKajiya_Kay(context, in->worldpos_tx.xyz, normalDir, in->tangent.xyz, viewWS, diffuseAcc, specularAcc);

	// �ضϵ�0-1

	uint32 colspec = context->Tex2D( tc0, 1 );
	float4 matSpec = uint32_2_float4(colspec);

	//diffuseAcc = (diffuseAcc + 0.5f) * matDiff;
	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;
	diffuseAcc += cBuffer->spcColor * specularAcc * matSpec;
	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	// srgb��ԭ
	diffuseAcc.sqrt();

	*out = float4_2_uint32(diffuseAcc);
}
