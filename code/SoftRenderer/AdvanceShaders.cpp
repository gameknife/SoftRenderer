/**
  @file SrCustomShader.cpp
  
  @author yikaiming

  history
  ver:1.0
   
 */

#include "stdafx.h"
#include "AdvanceShaders.h"

SrSkinSimShader g_SkinSimShader;
SrFresnelNormalShader g_FresnelNormalShader;

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

	float2 uv1 = float2::make(inTHREE[0]->worldpos_tx.w, inTHREE[0]->normal_ty.w);
	float2 uv2 = float2::make(inTHREE[1]->worldpos_tx.w, inTHREE[1]->normal_ty.w);
	float2 uv3 = float2::make(inTHREE[2]->worldpos_tx.w, inTHREE[2]->normal_ty.w);

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


	// 坐标变换
	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;

	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;

	out->normal_ty = float4::make( (context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);

	out->tangent.xyz = in->pos.xyz;// * out->pos.w;
}

void SrSkinSimShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

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
 *@remark
 */
void SrSkinSimShader::ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const
{
	SrFresnelNormal_Vert2Frag* in = (SrFresnelNormal_Vert2Frag*)pIn;
	uint32* out = (uint32*)pOut;
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	float2 tc0 = float2::make(in->worldpos_tx.w, in->normal_ty.w);

	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	col = context->Tex2D( tc0, 4 );
	float4 matSSS = uint32_2_float4(col);

	matDiff = matDiff * matDiff;

	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();
	float3 tangentDir = in->tangent.xyz;
	tangentDir.normalize();
	float3 binormalDir = tangentDir % normalDir;

	float33 tangent2world( tangentDir, binormalDir, normalDir );

	uint32 nor = context->Tex2D( tc0, 1 );
	float4 norf = uint32_2_float4(nor);

	float3 normalTangent = norf.xyz;
	normalTangent = (normalTangent - float3::make(0.5f)) * 2.f;

	normalDir = tangent2world * normalTangent;
	normalDir.normalize();
	
	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	float3 viewReflWS;
	viewReflWS.reflect(viewWS, normalDir);

	float2 calcTexcoord = viewReflWS.xy * 0.5f + float2::make(0.5f, 0.5f);
	uint32 refl = context->Tex2D( calcTexcoord, 3 );
	float4 reflf = uint32_2_float4(refl);

	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc = float4::make(0.f);

	CalcLightsSkin(context, in->worldpos_tx.xyz, normalDir, viewWS, matSSS, diffuseAcc, specularAcc);

	
	float NdotE = float3::dot(viewWS, normalDir);

	float rim = GetFresnel(NdotE, 0.01f, 5.f, 20.f);

	specularAcc += (reflf * diffuseAcc) * rim;

	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;

	diffuseAcc += (cBuffer->spcColor * matDiff.a) * (specularAcc) ;

	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	diffuseAcc.sqrt();

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

	float2 uv1 = float2::make(inTHREE[0]->worldpos_tx.w, inTHREE[0]->normal_ty.w);
	float2 uv2 = float2::make(inTHREE[1]->worldpos_tx.w, inTHREE[1]->normal_ty.w);
	float2 uv3 = float2::make(inTHREE[2]->worldpos_tx.w, inTHREE[2]->normal_ty.w);

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

	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;

	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;

	out->normal_ty = float4::make( (context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);

	out->tangent.xyz = in->pos.xyz;
}

void SrFresnelNormalShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

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

	float2 tc0 = float2::make(in->worldpos_tx.w, in->normal_ty.w);
	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	matDiff = matDiff * matDiff;

	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();
	float3 tangentDir = in->tangent.xyz;
	tangentDir.normalize();
	float3 binormalDir = tangentDir % normalDir;

	float33 tangent2world( tangentDir, binormalDir, normalDir );

	uint32 nor = context->Tex2D( tc0, 1 );
	float4 norf = uint32_2_float4(nor);

	float3 normalTangent = norf.xyz;
	normalTangent = (normalTangent - float3::make(0.5f)) * 2.f;

	normalDir = tangent2world * normalTangent;
	normalDir.normalize();

	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	float3 viewReflWS;
	viewReflWS.reflect(viewWS, normalDir);

	float2 calcTexcoord = viewReflWS.xy * 0.5f + float2::make(0.5f, 0.5f);
	uint32 refl = context->Tex2D( calcTexcoord, 3 );
	float4 reflf = uint32_2_float4(refl) * 0.25f;


	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc = float4::make(0.f);

	CalcLights(context, in->worldpos_tx.xyz, normalDir, viewWS, diffuseAcc, specularAcc);

	float NdotE = float3::dot(viewWS, normalDir);
	float fresnel = GetFresnel(NdotE, cBuffer->fresnelBia, cBuffer->fresnelPower, cBuffer->fresnelScale);

	specularAcc += reflf * diffuseAcc;

	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;
	diffuseAcc += ((cBuffer->spcColor) * specularAcc) * fresnel;
	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	diffuseAcc.sqrt();

	*out = float4_2_uint32(diffuseAcc);
}
