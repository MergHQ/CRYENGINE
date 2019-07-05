// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryRenderer/IStereoRenderer.h>

#include <string>
#include <atomic>
#include <mutex>

class CD3D9Renderer;

#if defined(INCLUDE_VR_RENDERING)

class CD3DHmdEmulatorRenderer : public IHmdRenderer
{
public:
	CD3DHmdEmulatorRenderer(IHmdDevice* device, CD3D9Renderer* renderer, CD3DStereoRenderer* stereoRenderer);
	virtual ~CD3DHmdEmulatorRenderer() = default;

	// IHDMRenderer
	virtual bool                      Initialize(int initialWidth, int initialeight) final;
	virtual void                      Shutdown() final;
	virtual void                      OnResolutionChanged(int newWidth, int newHeight) final;
	virtual void                      ReleaseBuffers() final {}
	virtual void                      PrepareFrame(uint64_t frameId) final;
	virtual void                      SubmitFrame() final;

	virtual RenderLayer::CProperties*  GetQuadLayerProperties(RenderLayer::EQuadLayers id) final;
	virtual RenderLayer::CProperties*  GetSceneLayerProperties(RenderLayer::ESceneLayers id) final;
	virtual std::pair<CTexture*, Vec4> GetMirrorTexture(EEyeType eye) const final;
	//~ IHDMRenderer

private:
	// Structure used for rendering the scene to the buffer of textures sent to the Hmd
	struct STextureSwapChainRenderData
	{
		Vec2i                             viewportPosition;
		Vec2i                             viewportSize;
		std::vector<_smart_ptr<CTexture>> textures;
	};

	struct TextureDesc
	{
		uint32 width;
		uint32 height;
		DXGI_FORMAT format;
	};

	bool InitializeTextureSwapSet(ID3D11Device* d3dDevice, EEyeType eye, STextureSwapChainRenderData& eyeRenderData, TextureDesc desc, const std::string& name);
	bool InitializeTextureSwapSet(ID3D11Device* d3dDevice, EEyeType eye, TextureDesc desc, const std::string& name);
	bool InitializeQuadTextureSwapSet(ID3D11Device* d3dDevice, RenderLayer::EQuadLayers id, TextureDesc desc, const std::string& name);

	void SetupRenderTargets();

private:
	// Helper to manage the layers in the Renderer
	// TODO: Depending on the layer future support by other vendors, this could become a separate system with its own interface
	struct SLayersManager
	{
		SLayersManager();

		RenderLayer::CProperties         m_scene3DLayerProperties[RenderLayer::eSceneLayers_Total];
		RenderLayer::CProperties         m_quadLayerProperties[RenderLayer::eQuadLayers_Total];
	};

private:
	IHmdDevice*                   m_pDevice;
	STextureSwapChainRenderData   m_scene3DRenderData[eEyeType_NumEyes];
	STextureSwapChainRenderData   m_quadLayerRenderData[RenderLayer::eQuadLayers_Total];

	SLayersManager                m_layerManager;

	uint32                        m_eyeWidth;
	uint32                        m_eyeHeight;

	CD3D9Renderer*                m_pRenderer;
	CD3DStereoRenderer*           m_pStereoRenderer;

	// Flag that is raised in case device was lost during frame preparation or submission
	std::atomic<bool>             m_deviceLostFlag{ true };

	CFullscreenPass m_copyPass;
};

#endif //defined(INCLUDE_VR_RENDERING)