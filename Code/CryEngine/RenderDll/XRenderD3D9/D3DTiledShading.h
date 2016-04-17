// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================

   Revision history:
   Created Jan 2013 by Nicolas Schulz

   =============================================================================*/

#ifndef _D3DTILEDSHADING_H_
#define _D3DTILEDSHADING_H_

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
	Vec4     shadowChannelIndex;
	Matrix44 projectorMatrix;
	Matrix44 shadowMatrix;
};  // 256 bytes

class CTiledShading
{
protected:
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
	CTiledShading();

	void                  CreateResources();
	void                  DestroyResources(bool destroyResolutionIndependentResources);
	void                  Clear();

	void                  Render(CRenderView* pRenderView, Vec4* clipVolumeParams);

	void                  BindForwardShadingResources(CShader* pShader, CDeviceManager::SHADER_TYPE shType = CDeviceManager::TYPE_PS);
	void                  UnbindForwardShadingResources(CDeviceManager::SHADER_TYPE shType = CDeviceManager::TYPE_PS);

	STiledLightShadeInfo* GetTiledLightShadeInfo();

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
	CGpuBuffer     m_tileLightIndexBuf;

	CGpuBuffer     m_clipVolumeInfoBuf;

	TextureAtlas   m_specularProbeAtlas;
	TextureAtlas   m_diffuseProbeAtlas;
	TextureAtlas   m_spotTexAtlas;

	uint32         m_nTexStateCompare;

	uint32         m_numSkippedLights;
	uint32         m_numAtlasUpdates;

	TArray<uint32> m_arrShadowCastingLights;

	bool           m_bApplyCaustics;
};

#endif
