// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================

   Revision history:
   Created Jan 2013 by Nicolas Schulz

   =============================================================================*/

#ifndef _D3DTILEDSHADING_H_
#define _D3DTILEDSHADING_H_

#include "GraphicsPipeline/Common/ComputeRenderPass.h"

const uint32 MaxNumTileLights = 255;
const uint32 LightTileSizeX = 8;
const uint32 LightTileSizeY = 8;

// Sun area light parameters (same as in standard deferred shading)
const float TiledShading_SunDistance = 10000.0f;
const float TiledShading_SunSourceDiameter = 94.0f;  // atan(AngDiameterSun) * 2 * SunDistance, where AngDiameterSun=0.54deg

struct STiledLightCullInfo
{
	uint32 volumeType;
	uint32 PADDING0;
	Vec2   depthBounds;
	Vec4   posRad;
	Vec4   volumeParams0;
	Vec4   volumeParams1;
	Vec4   volumeParams2;
};  // 80 bytes

struct STiledLightShadeInfo
{
	uint32   lightType;
	uint32   resIndex;
	uint32   shadowMaskIndex;
	uint16   stencilID0;
	uint16   stencilID1;
	Vec4     posRad;
	Vec2     attenuationParams;
	Vec2     shadowParams;
	Vec4     color;
	Matrix44 projectorMatrix;
	Matrix44 shadowMatrix;
};  // 192 bytes

class CTiledShading
{
protected:
	friend class CTiledShadingStage;
	friend class CSceneForwardStage;

	struct AtlasItem
	{
		ITexture* texture;
		int       updateFrameID;
		int       accessFrameID;
		bool      invalid;

		AtlasItem() : texture(NULL), updateFrameID(-1), accessFrameID(0), invalid(false) {}
	};

	struct TextureAtlas
	{
		CTexture*              texArray;
		std::vector<AtlasItem> items;

		TextureAtlas() : texArray(NULL) {}
	};

public:
	enum ETiledVolumeTypes
	{
		tlVolumeSphere = 1,
		tlVolumeCone   = 2,
		tlVolumeOBB    = 3,
		tlVolumeSun    = 4,
	};
	
	CTiledShading();

	void                  CreateResources();
	void                  DestroyResources(bool destroyResolutionIndependentResources);
	void                  Clear();

	void                  Render(CRenderView* pRenderView, Vec4* clipVolumeParams);

	void                  BindForwardShadingResources(CShader* pShader, CDeviceManager::SHADER_TYPE shType = CDeviceManager::TYPE_PS);
	void                  UnbindForwardShadingResources(CDeviceManager::SHADER_TYPE shType = CDeviceManager::TYPE_PS);

	template<class RenderPassType>
	void                  BindForwardShadingResources(RenderPassType& pass);

	STiledLightCullInfo*  GetTiledLightCullInfo();
	STiledLightShadeInfo* GetTiledLightShadeInfo();
	uint32                GetValidLightCount()                                                 { return m_numValidLights; }

	int                   InsertTextureToSpecularProbeAtlas(CTexture* texture, int arrayIndex) { return InsertTexture(texture, m_specularProbeAtlas, arrayIndex); }
	int                   InsertTextureToDiffuseProbeAtlas(CTexture* texture, int arrayIndex)  { return InsertTexture(texture, m_diffuseProbeAtlas, arrayIndex); }
	int                   InsertTextureToSpotTexAtlas(CTexture* texture, int arrayIndex)       { return InsertTexture(texture, m_spotTexAtlas, arrayIndex); }

	void                  NotifyCausticsVisible()                                              { m_bApplyCaustics = true; }

protected:
	int  InsertTexture(CTexture* texture, TextureAtlas& atlas, int arrayIndex);
	void PrepareLightList(CRenderView* pRenderView);
	void PrepareShadowCastersList(CRenderView* pRenderView);
	void PrepareClipVolumeList(Vec4* clipVolumeParams);

protected:
	uint32         m_dispatchSizeX, m_dispatchSizeY;

	CGpuBuffer     m_lightCullInfoBuf;
	CGpuBuffer     m_LightShadeInfoBuf;
	CGpuBuffer     m_tileOpaqueLightMaskBuf;
	CGpuBuffer     m_tileTranspLightMaskBuf;

	CGpuBuffer     m_clipVolumeInfoBuf;

	TextureAtlas   m_specularProbeAtlas;
	TextureAtlas   m_diffuseProbeAtlas;
	TextureAtlas   m_spotTexAtlas;

	uint32         m_samplerTrilinearClamp;
	uint32         m_samplerCompare;

	uint32         m_numValidLights;
	uint32         m_numSkippedLights;
	uint32         m_numAtlasUpdates;

	TArray<uint32> m_arrShadowCastingLights;

	bool           m_bApplyCaustics;
};

#endif
