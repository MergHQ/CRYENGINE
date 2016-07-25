// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifdef __cplusplus
// C++
	#pragma once
	#pragma pack(push,4)

	#define hlsl_cbuffer(name) struct HLSL_ ## name
	#define hlsl_cbuffer_register(name, reg, slot) \
	  enum { SLOT_ ## name = slot };               \
	  struct HLSL_ ## name

	#define hlsl_uint(member)     uint member
	#define hlsl_float(member)    float member
	#define hlsl_float2(member)   Vec2 member
	#define hlsl_float3(member)   Vec3 member
	#define hlsl_float4(member)   Vec4 member
	#define hlsl_matrix44(member) Matrix44 member
	#define hlsl_matrix34(member) Matrix34 member
#else //__cplusplus
// HLSL
	#define hlsl_cbuffer(name)                      cbuffer name
	#define hlsl_cbuffer_register(name, reg, float) cbuffer name: reg
	#define hlsl_uint(member)                       uint member
	#define hlsl_float(member)                      float member
	#define hlsl_float2(member)                     float2 member
	#define hlsl_float3(member)                     float3 member
	#define hlsl_float4(member)                     float4 member
	#define hlsl_matrix44(member)                   float4x4 member
	#define hlsl_matrix34(member)                   float3x4 member
#endif //__cplusplus

// TODO: include in shaders

hlsl_cbuffer(PerPassConstantBuffer_GBuffer)
{
	hlsl_float4(g_VS_WorldViewPos);
	hlsl_matrix44(g_VS_ViewProjMatr);
	hlsl_matrix44(g_VS_ViewProjZeroMatr);
};

hlsl_cbuffer(PerPassConstantBuffer_ShadowGen)
{
	hlsl_float4(CP_ShadowGen_LightPos);
	hlsl_float4(CP_ShadowGen_ViewPos);

	hlsl_float4(CP_ShadowGen_DepthTestBias);
	hlsl_float4(CP_ShadowGen_FrustrumInfo);

	hlsl_float4(CP_ShadowGen_VegetationAlphaClamp);
};

hlsl_cbuffer_register(PerInstanceConstantBuffer_Base, register (b12), 12) // eConstantBufferShaderSlot_PerInstance
{
	hlsl_matrix34(SPIObjWorldMat);
	hlsl_matrix34(SPIPrevObjWorldMat);
	hlsl_float4(SPIBendInfo);
	hlsl_float4(SPIAlphaTest);
};

hlsl_cbuffer_register(PerInstanceConstantBuffer_TerrainVegetation, register (b12), 12) // eConstantBufferShaderSlot_PerInstance
{
	hlsl_matrix34(SPIObjWorldMat);
	hlsl_matrix34(SPIPrevObjWorldMat);
	hlsl_float4(SPIBendInfo);
	hlsl_float4(SPIAlphaTest);

	hlsl_float4(BlendTerrainColInfo);
	hlsl_matrix44(TerrainLayerInfo);
};

hlsl_cbuffer_register(PerInstanceConstantBuffer_Skin, register (b12), 12) // eConstantBufferShaderSlot_PerInstance
{
	hlsl_matrix34(SPIObjWorldMat);
	hlsl_matrix34(SPIPrevObjWorldMat);
	hlsl_float4(SPIBendInfo);
	hlsl_float4(SPIAlphaTest);

	hlsl_float4(SkinningInfo);
	hlsl_float4(WrinklesMask0);
	hlsl_float4(WrinklesMask1);
	hlsl_float4(WrinklesMask2);
};

hlsl_cbuffer_register(PerViewGlobalConstantBuffer, register (b13), 13) //eConstantBufferShaderSlot_PerView
{
	hlsl_matrix44(CV_ViewProjZeroMatr);
	hlsl_float4(CV_AnimGenParams);

	hlsl_matrix44(CV_ViewProjMatr);
	hlsl_matrix44(CV_ViewProjNearestMatr);
	hlsl_matrix44(CV_InvViewProj);
	hlsl_matrix44(CV_PrevViewProjMatr);
	hlsl_matrix44(CV_PrevViewProjNearestMatr);
	hlsl_matrix34(CV_ScreenToWorldBasis);
	hlsl_float4(CV_TessInfo);
	hlsl_float4(CV_CamFrontVector);
	hlsl_float4(CV_CamUpVector);

	hlsl_float4(CV_ScreenSize);
	hlsl_float4(CV_HPosScale);
	hlsl_float4(CV_ProjRatio);
	hlsl_float4(CV_NearestScaled);
	hlsl_float4(CV_NearFarClipDist);

	hlsl_float4(CV_SunLightDir);
	hlsl_float4(CV_SunColor);
	hlsl_float4(CV_SkyColor);
	hlsl_float4(CV_FogColor);
	hlsl_float4(CV_TerrainInfo);

	hlsl_float4(CV_DecalZFightingRemedy);

	hlsl_matrix44(CV_FrustumPlaneEquation);

	hlsl_float4(CV_ShadowLightPos);
	hlsl_float4(CV_ShadowViewPos);

	hlsl_float4(CV_WindGridOffset);
};

struct SLightVolumeInfo
{
	hlsl_float3(wPosition);
	hlsl_float(radius);
	hlsl_float3(cColor);
	hlsl_float(bulbRadius);
	hlsl_float3(wProjectorDirection);
	hlsl_float(projectorCosAngle);
};

struct SLightVolumeRange
{
	hlsl_uint(begin);
	hlsl_uint(end);
};

#ifdef __cplusplus
// C++

	#pragma pack(pop)

#else //__cplusplus
// HLSL

#endif //__cplusplus
