#pragma once

//////////////////////////////////////////////////////////////////////////
// 枚举

/**
*@brief VB格式, 暂时只使用P3N3T2
*/
enum ESrVertDecl
{
	// data struct ALIGNED
	eVd_Invalid = 0,
	eVd_F4F4,
	eVd_F4F4F4,
	eVd_F4F4F4F4U4,

	eVd_Max,
};

/**
*@brief 矩阵组
*/
enum EMatrixDefine
{
	eMd_WorldViewProj = 0,
	eMd_World,
	eMd_View,
	eMd_Projection,
	eMd_WorldInverse,
	eMd_ViewInverse,
	eMd_Count,
};

/**
*@brief 光栅化方式
*/
enum ERasterizeMode
{
	eRm_Solid,
	eRm_WireFrame,
	eRm_Point,
};

/**
*@brief 采样滤镜
*/
enum ESamplerFilter
{
	eSF_Nearest,		///< 临近点采样
	eSF_Linear,			///< 双线性过滤
};

/**
*@brief 渲染器状态
*/
enum ERenderingState
{
	eRs_Rendering = 1 << 0,
	eRS_Locked = 1 << 1,
	eRS_Swaping = 1 << 2,
};

/**
*@brief 渲染特性
*/
enum ERenderFeature
{
	eRFeature_JitAA = 1 << 0,						///< 抖动抗锯齿
	eRFeature_MThreadRendering = 1 << 1,			///< 多线程渲染
	eRFeature_LinearFiltering = 1 << 2,			///< 双线性采样
	eRFeature_DotCoverageRendering = 1 << 3,		///< Dot空洞渲染
	eRFeature_InterlaceRendering = 1 << 4,		///< Dot空洞渲染
};

/**
*@brief 资源类型
*/
enum EResourceType
{
	eRt_Mesh = 0,
	eRT_Texture,
	eRT_Material,
	eRT_Shader,

	eRT_Count,
};

enum EShaderConstantsSlot
{
	eSC_VS0 = 0,
	eSC_VS1,
	eSC_VS2,
	eSC_VS3,

	eSC_PS0 = SR_SHADER_CONSTANTS_NUM,
	eSC_PS1,
	eSC_PS2,
	eSC_PS3,

	eSC_ShaderConstantCount = SR_SHADER_CONSTANTS_NUM * 2,
};