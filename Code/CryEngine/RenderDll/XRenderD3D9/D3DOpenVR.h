// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_VR_RENDERING)

	#include <CrySystem/VR/IHMDDevice.h>
	#include <../CryPlugins/VR/CryOpenVR/Interface/IHmdOpenVRDevice.h>
	#include <CryRenderer/IStereoRenderer.h>

class CD3D9Renderer;

class CD3DOpenVRRenderer : public IHmdRenderer
{
public:
	CD3DOpenVRRenderer(CryVR::OpenVR::IOpenVRDevice* openVRDevice, CD3D9Renderer* renderer, CD3DStereoRenderer* stereoRenderer);
	~CD3DOpenVRRenderer();

	// IHDMRenderer
	virtual bool                      Initialize() override;
	virtual void                      Shutdown() override;
	virtual void                      OnResolutionChanged() override;
	virtual void                      ReleaseBuffers() override;
	virtual void                      PrepareFrame() override;
	virtual void                      SubmitFrame() override;
	virtual void                      RenderSocialScreen() override;
	virtual RenderLayer::CProperties* GetQuadLayerProperties(RenderLayer::EQuadLayers id) override;
	virtual RenderLayer::CProperties* GetSceneLayerProperties(RenderLayer::ESceneLayers id) override { return nullptr; }
	// ~IHDMRenderer

protected:
	struct TextureDesc
	{
		uint32 width;
		uint32 height;
		uint32 format;
	};

protected:
	bool             InitializeEyeTarget(D3DDevice* d3dDevice, EEyeType eye, TextureDesc desc, const char* name);
	bool             InitializeQuadLayer(D3DDevice* d3dDevice, int quadLayer, TextureDesc desc, const char* name);
	bool             InitializeMirrorTexture(D3DDevice* d3dDevice, EEyeType eye, TextureDesc desc, const char* name);

protected:
	struct Eye
	{
		CTexture* texture;
	} m_eyes[EEyeType::eEyeType_NumEyes];
	struct QuadLayer
	{
		CTexture* texture;
	}                             m_quads[RenderLayer::eQuadLayers_Total];

	RenderLayer::CProperties      m_quadLayerProperties[RenderLayer::eQuadLayers_Total];

	uint32                        m_numFrames;
	uint32                        m_currentFrame;

	CTexture*                     m_mirrorTextures[EEyeType::eEyeType_NumEyes];

	uint32                        m_uEyeWidth;
	uint32                        m_uEyeHeight;

	CryVR::OpenVR::IOpenVRDevice* m_openVRDevice;
	CD3D9Renderer*                m_renderer;
	CD3DStereoRenderer*           m_stereoRenderer;

	CCryNameR                     m_Param0Name;
	CCryNameR                     m_Param1Name;
	CCryNameTSCRC                 m_textureToTexture;
};

#endif //defined(INCLUDE_VR_RENDERING)
