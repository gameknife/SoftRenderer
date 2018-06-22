/**
  @file SrCustomShader.cpp
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#include "stdafx.h"
#include "AdvanceShaders.h"

#include "mmgr/mmgr.h"

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


	// pos处理到投影空间
	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;
	// pos处理到世界空间，保存
	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;
	// normal,tangent处理到世界空间
	out->normal_ty = float4( (context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);

	// 暂时把pos存入tangent，供geometry阶段计算tangent
	out->tangent.xyz = in->pos.xyz;// * out->pos.w;
	//out->texcoord2 = in->texcoord * out->pos.w;
}

void SrSkinSimShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	// 线性插值project space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// 已经除w
	// 直接插值，其他channel
	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

	// 对于scanline扫描的，将透视插值坐标，插值回正常值
	if (final)
	{
		verO->normal_ty /= verO->pos.w;
		verO->worldpos_tx /= verO->pos.w;
		verO->tangent /= verO->pos.w;
	}
}

/**
 *@brief Skin模拟Pixel Shader
 *@return void 
 *@param uint32 * pOut 
 *@param const void * pIn 
 *@param const SrShaderContext * context 
 *@remark 使用nvLamb，利用NdotL求取次表面散射的模拟穿透值，从次表面颜色图上采样对应的颜色，作为光照的补充。
 同时，利用NdotE，求取边缘的强度，乘以环境颜色以表现皮肤的通透感。
 */
void SrSkinSimShader::ProcessPixel( uint32* pOut, const void* pIn, const SrShaderContext* context, uint32 address ) const
{
	SrFresnelNormal_Vert2Frag* in = (SrFresnelNormal_Vert2Frag*)pIn;
	uint32* out = (uint32*)pOut;
	SrPixelShader_Constants* cBuffer = (SrPixelShader_Constants*)(context->GetPixelShaderConstantPtr());

	// 采样diffuse颜色
	float2 tc0(in->worldpos_tx.w, in->normal_ty.w);

	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	// 采样次表面颜色
	col = context->Tex2D( tc0, 4 );
	float4 matSSS = uint32_2_float4(col);

	// diffuse颜色 作 srgb
	matDiff = matDiff * matDiff;

	// 法线扰动处理
	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();
	float3 tangentDir = in->tangent.xyz;
	tangentDir.normalize();
	float3 binormalDir = tangentDir % normalDir;

	// 利用世界空间的 切线，法线，副法线 构造从切线空间到世界空间的转换矩阵
	float33 tangent2world( tangentDir, binormalDir, normalDir );

	// 采样normal
	uint32 nor = context->Tex2D( tc0, 1 );
	float4 norf = uint32_2_float4(nor);

	// 将0-1处理到-1到1空间
	float3 normalTangent = norf.xyz;
	normalTangent = (normalTangent - float3(0.5f)) * 2.f;

	// 将采样到的切线空间法线转换到世界空间
	normalDir = tangent2world * normalTangent;
	normalDir.normalize();
	
	// 视线方向计算
	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	// 视线反射方向
	float3 viewReflWS;
	viewReflWS.reflect(viewWS, normalDir);

	// 采样反射纹理
	uint32 refl = context->Tex2D( (viewReflWS.xy * 0.5f + float2(0.5f, 0.5f)), 3 );
	float4 reflf = uint32_2_float4(refl);

	// 取得全局天光，并利用normal作快速天光模拟
	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc(0.f);

	// 调用shading lib的皮肤光照计算函数，计算diffuse和specular的光照累积
	CalcLightsSkin(context, in->worldpos_tx.xyz, normalDir, viewWS, matSSS, diffuseAcc, specularAcc);

	
	float NdotE = float3::dot(viewWS, normalDir);
	//float fresnel = GetFresnel(NdotE, cBuffer->fresnelBia, cBuffer->fresnelPower, cBuffer->fresnelScale);
	float rim = GetFresnel(NdotE, 0.01f, 5.f, 20.f);

	// 光照方程1：材质固有色 * 漫反射和环境的光照聚集
	specularAcc += (reflf * diffuseAcc) * rim;

	// rim light
	//specularAcc += ( float4( 1.f, 1.f, 0.95686f, 0.847f ) * diffuseAcc ) * rim;

	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;
	// 光照方程2：材质反射色 * 镜面反射光照聚集
	diffuseAcc += (cBuffer->spcColor * matDiff.a) * (specularAcc) ;

	// 最终色彩截断到0-1
	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	// srgb还原
	diffuseAcc.sqrt();

	// float4 到 uint32的输出
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

	// pos处理到投影空间
	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;
	// pos处理到世界空间，保存
	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;
	// normal,tangent处理到世界空间
	out->normal_ty = float4( (context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);

	// 暂时把pos存入tangent，供geometry阶段计算tangent
	out->tangent.xyz = in->pos.xyz;// * out->pos.w;
	//out->texcoord2 = in->texcoord * out->pos.w;
}

void SrFresnelNormalShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	// 线性插值project space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// 已经除w
	// 直接插值，其他channel
	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

	// 对于scanline扫描的，将透视插值坐标，插值回正常值
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

	// 采样diffuse颜色
	float2 tc0(in->worldpos_tx.w, in->normal_ty.w);
	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	// diffuse 作 srgb
	matDiff = matDiff * matDiff;

	// 法线扰动处理
	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();
	float3 tangentDir = in->tangent.xyz;
	tangentDir.normalize();
	float3 binormalDir = tangentDir % normalDir;

	// 利用世界空间的 切线，法线，副法线 构造从切线空间到世界空间的转换矩阵
	float33 tangent2world( tangentDir, binormalDir, normalDir );

	// 采样normal
	uint32 nor = context->Tex2D( tc0, 1 );
	float4 norf = uint32_2_float4(nor);

	float3 normalTangent = norf.xyz;
	normalTangent = (normalTangent - float3(0.5f)) * 2.f;
	//normalTangent.y *= -1;

	// 将采样到的切线空间法线转换到世界空间
	normalDir = tangent2world * normalTangent;
	normalDir.normalize();

	// 视线方向计算
	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	// 视线反射方向
	float3 viewReflWS;
	viewReflWS.reflect(viewWS, normalDir);

	// 采样反射纹理
	uint32 refl = context->Tex2D( (viewReflWS.xy * 0.5f + float2(0.5f, 0.5f)), 3 );
	float4 reflf = uint32_2_float4(refl) * 0.25f;


	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc(0.f);

	CalcLights(context, in->worldpos_tx.xyz, normalDir, viewWS, diffuseAcc, specularAcc);

	float NdotE = float3::dot(viewWS, normalDir);
	float fresnel = GetFresnel(NdotE, cBuffer->fresnelBia, cBuffer->fresnelPower, cBuffer->fresnelScale);

	specularAcc += reflf * diffuseAcc;
	// 截断到0-1
	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;
	diffuseAcc += ((cBuffer->spcColor) * specularAcc) * fresnel;
	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	// srgb还原
	diffuseAcc.sqrt();

	*out = float4_2_uint32(diffuseAcc);
}

void SRFASTCALL SrHairShader::ProcessVertex( void* vOut, void* vOut1, void* vOut2, const void* vInRef0, const void* vInRef1, const void* vInRef2, const SrShaderContext* context ) const
{
	SrVertexP3N3T2* in = (SrVertexP3N3T2*)vInRef0;
	SrFresnelNormal_Vert2Frag* out = (SrFresnelNormal_Vert2Frag*)vOut;

	// pos处理到投影空间
	out->pos = context->matrixs[eMd_WorldViewProj] * in->pos;
	// pos处理到世界空间，保存
	out->worldpos_tx = (context->matrixs[eMd_World] * in->pos);
	out->worldpos_tx.w = in->texcoord.x;
	// normal,tangent处理到世界空间
	out->normal_ty = float4((context->matrixs[eMd_World].RotateVector3(in->normal)), in->texcoord.y);
}

void SRFASTCALL SrHairShader::ProcessRasterize( void* rOut, const void* rInRef0, const void* rInRef1, const void* rInRef2, float ratio, const SrShaderContext* context, bool final /*= false */ ) const
{
	const SrFresnelNormal_Vert2Frag* verA = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef0);
	const SrFresnelNormal_Vert2Frag* verB = static_cast<const SrFresnelNormal_Vert2Frag*>(rInRef1);
	SrFresnelNormal_Vert2Frag* verO = static_cast<SrFresnelNormal_Vert2Frag*>(rOut);

	// 线性插值project space pos
	float inv_ratio = 1.f - ratio;
	verO->pos = SrFastLerp( verA->pos, verB->pos, ratio, inv_ratio );

	// 已经除w
	// 直接插值，其他channel
	verO->normal_ty = SrFastLerp( verA->normal_ty, verB->normal_ty, ratio, inv_ratio );
	verO->worldpos_tx = SrFastLerp( verA->worldpos_tx, verB->worldpos_tx, ratio, inv_ratio );
	verO->tangent = SrFastLerp( verA->tangent, verB->tangent, ratio, inv_ratio );

	// 对于scanline扫描的，将透视插值坐标，插值回正常值
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

	// 采样diffuse颜色
	float2 tc0( in->worldpos_tx.w, in->normal_ty.w );
	uint32 col = context->Tex2D( tc0, 0 );
	float4 matDiff = uint32_2_float4(col);

	// diffuse 作 srgb
	matDiff = matDiff * matDiff;

	// 法线扰动处理
	float3 normalDir = in->normal_ty.xyz;
	normalDir.normalize();

	// 	*out = float4_2_uint32(float4((normalDir * 0.5f + float3(0.5f)),1.f));
	// 	return;


	// 视线方向计算
	float3 viewWS = context->matrixs[eMd_ViewInverse].GetTranslate() - in->worldpos_tx.xyz;
	viewWS.normalize();

	float4 diffuseAcc = gEnv->sceneMgr->GetSkyLightColor() * (normalDir.y * 0.4f + 0.6f);
	float4 specularAcc(0.f);

	CalcLightsKajiya_Kay(context, in->worldpos_tx.xyz, normalDir, in->tangent.xyz, viewWS, diffuseAcc, specularAcc);

	// 截断到0-1

	uint32 colspec = context->Tex2D( tc0, 1 );
	float4 matSpec = uint32_2_float4(colspec);

	//diffuseAcc = (diffuseAcc + 0.5f) * matDiff;
	diffuseAcc = diffuseAcc * matDiff * cBuffer->difColor;
	diffuseAcc += cBuffer->spcColor * specularAcc * matSpec;
	diffuseAcc = Clamp(diffuseAcc, 0.f, 1.f);	

	// srgb还原
	diffuseAcc.sqrt();

	*out = float4_2_uint32(diffuseAcc);
}
