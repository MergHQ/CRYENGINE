// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "D3DHmdEmulator.h"
#include "D3DPostProcess.h"

#include <Common/RenderDisplayContext.h>

#include <CrySystem/VR/IHMDManager.h>
#include <CrySystem/VR/IHMDDevice.h>
#ifdef ENABLE_BENCHMARK_SENSOR
	#include <IBenchmarkFramework.h>
	#include <IBenchmarkRendererSensorManager.h>
#endif
#if defined(INCLUDE_VR_RENDERING)

CD3DHmdEmulatorRenderer::SLayersManager::SLayersManager()
{
	// Init layers properties
	for (uint32 i = RenderLayer::eSceneLayers_0; i < RenderLayer::eSceneLayers_Total; ++i)
	{
		m_scene3DLayerProperties[i].SetType(RenderLayer::eLayer_Scene3D);
		m_scene3DLayerProperties[i].SetPose(QuatTS(Quat(IDENTITY), Vec3(0.f, 0.f, -.8f), 1.f));
		m_scene3DLayerProperties[i].SetId(i);
	}

	for (uint32 i = RenderLayer::eQuadLayers_0; i < RenderLayer::eQuadLayers_Headlocked_0; ++i)
	{
		m_quadLayerProperties[i].SetType(RenderLayer::eLayer_Quad);
		m_quadLayerProperties[i].SetPose(QuatTS(Quat(IDENTITY), Vec3(0.f, 0.f, -.8f), 1.f));
		m_quadLayerProperties[i].SetId(i);
	}
	for (uint32 i = RenderLayer::eQuadLayers_Headlocked_0; i < RenderLayer::eQuadLayers_Total; ++i)
	{
		m_quadLayerProperties[i].SetType(RenderLayer::eLayer_Quad_HeadLocked);
		m_quadLayerProperties[i].SetPose(QuatTS(Quat(IDENTITY), Vec3(0.f, 0.f, -.8f), 1.f));
		m_quadLayerProperties[i].SetId(i);
	}
}

// -------------------------------------------------------------------------

CD3DHmdEmulatorRenderer::CD3DHmdEmulatorRenderer(IHmdDevice* device, CD3D9Renderer* renderer, CD3DStereoRenderer* stereoRenderer)
	: m_pDevice(device)
	, m_pRenderer(renderer)
	, m_pStereoRenderer(stereoRenderer)
	, m_eyeWidth(~0L)
	, m_eyeHeight(~0L)
	, m_copyPass(renderer->GetActiveGraphicsPipeline().get())
{
}

bool CD3DHmdEmulatorRenderer::Initialize(int initialWidth, int initialeight)
{
	ID3D11Device* pD3d11Device = m_pRenderer->GetDevice();
	
	m_eyeWidth  = initialWidth;
	m_eyeHeight = initialeight;

	D3DFormat texFmt = DeviceFormats::ConvertFromTexFormat(CRendererResources::GetLDRFormat(true));

	TextureDesc eyeTextureDesc;
	eyeTextureDesc.width = m_eyeWidth;
	eyeTextureDesc.height = m_eyeHeight;
	eyeTextureDesc.format = texFmt;

	TextureDesc quadTextureDesc;
	quadTextureDesc.width = m_eyeWidth;
	quadTextureDesc.height = m_eyeHeight;
	quadTextureDesc.format = texFmt;

	if (!InitializeTextureSwapSet(pD3d11Device, EEyeType::eEyeType_LeftEye, eyeTextureDesc, "$LeftEye_") ||
	    !InitializeTextureSwapSet(pD3d11Device, EEyeType::eEyeType_RightEye, eyeTextureDesc, "$RightEye_") ||
	    !InitializeQuadTextureSwapSet(pD3d11Device, RenderLayer::eQuadLayers_0, quadTextureDesc, "$QuadLayer0_") ||
		!InitializeQuadTextureSwapSet(pD3d11Device, RenderLayer::eQuadLayers_Headlocked_0, quadTextureDesc, "$QuadLayerHeadLocked0_") ||
		!InitializeQuadTextureSwapSet(pD3d11Device, RenderLayer::eQuadLayers_Headlocked_1, quadTextureDesc, "$QuadLayerHeadLocked1_"))
	{
		Shutdown();
		return false;
	}

	// Create display contexts for eyes
	for (uint32 eye = 0; eye < eEyeType_NumEyes; ++eye)
	{
		std::vector<_smart_ptr<CTexture>> swapChain;
		for (const auto& tex : m_scene3DRenderData[eye].textures)
			swapChain.emplace_back(tex.get());
		m_pStereoRenderer->CreateEyeDisplayContext(CCamera::EEye(eye), std::move(swapChain));
	}
	// Create display contexts for quad layers
	for (uint32 quad = 0; quad < RenderLayer::eQuadLayers_Total; ++quad)
	{
		std::vector<_smart_ptr<CTexture>> swapChain;
		for (const auto& tex : m_quadLayerRenderData[quad].textures)
			swapChain.emplace_back(tex.get());
		m_pStereoRenderer->CreateVrQuadLayerDisplayContext(RenderLayer::EQuadLayers(quad), std::move(swapChain));
	}

	SetupRenderTargets();

	m_deviceLostFlag = false;

	return true;
}

bool CD3DHmdEmulatorRenderer::InitializeTextureSwapSet(ID3D11Device* pD3d11Device, EEyeType eye, STextureSwapChainRenderData& eyeRenderData, CD3DHmdEmulatorRenderer::TextureDesc desc, const std::string& name)
{
	_smart_ptr<CTexture> eyeTexture = CTexture::GetOrCreateRenderTarget((name + "Buffer").c_str(), desc.width, desc.height, Clr_Neutral, eTT_2D, FT_DONT_STREAM, DeviceFormats::ConvertToTexFormat(desc.format));

	if (eyeTexture == nullptr)
		return false;

	eyeRenderData.viewportPosition = Vec2i(0, 0);
	eyeRenderData.viewportSize = Vec2i(desc.width, desc.height);
	eyeRenderData.textures.emplace_back(std::move(eyeTexture));

	return true;
}

bool CD3DHmdEmulatorRenderer::InitializeTextureSwapSet(ID3D11Device* pD3d11Device, EEyeType eye, CD3DHmdEmulatorRenderer::TextureDesc  desc, const std::string& name)
{
	return InitializeTextureSwapSet(pD3d11Device, eye, m_scene3DRenderData[eye], desc, name);
}

bool CD3DHmdEmulatorRenderer::InitializeQuadTextureSwapSet(ID3D11Device* d3dDevice, RenderLayer::EQuadLayers id, CD3DHmdEmulatorRenderer::TextureDesc desc, const std::string& name)
{
	return InitializeTextureSwapSet(d3dDevice, static_cast<EEyeType>(-1), m_quadLayerRenderData[id], desc, name);
}

void CD3DHmdEmulatorRenderer::Shutdown()
{
	for (uint32 eye = 0; eye < eEyeType_NumEyes; ++eye)
		m_pStereoRenderer->CreateEyeDisplayContext(CCamera::EEye(eye), {});
	for (uint32 quad = 0; quad < RenderLayer::eQuadLayers_Total; ++quad)
		m_pStereoRenderer->CreateVrQuadLayerDisplayContext(RenderLayer::EQuadLayers(quad), {});

	// Scene3D layers
	for (uint32 eye = 0; eye < 2; ++eye)
	{
		for (size_t t = 0; t < m_scene3DRenderData[eye].textures.size(); ++t)
			m_scene3DRenderData[eye].textures[t]->Release();
		m_scene3DRenderData[eye].textures = {};
	}

	// Quad layers
	for (uint32 i = 0; i < RenderLayer::eQuadLayers_Total; ++i)
	{
		for (size_t t = 0; t < m_quadLayerRenderData[i].textures.size(); ++t)
			m_scene3DRenderData[i].textures[t]->Release();
		m_quadLayerRenderData[i].textures = {};
	}

	ReleaseBuffers();

	m_deviceLostFlag = true;
}

void CD3DHmdEmulatorRenderer::OnResolutionChanged(int newWidth, int newHeight)
{
	if (m_eyeWidth  != newWidth ||
		m_eyeHeight != newHeight)
	{
		Shutdown();
		Initialize(newWidth, newHeight);
	}
}

void CD3DHmdEmulatorRenderer::PrepareFrame(uint64_t frameId)
{
	SetupRenderTargets();

	GetSceneLayerProperties(RenderLayer::eSceneLayers_0)->SetActive(true);

	// Recreate device if devicelost error was raised
	if (m_deviceLostFlag.load())
	{
		Shutdown();
		Initialize(m_eyeWidth, m_eyeHeight);
	}
}

void CD3DHmdEmulatorRenderer::SetupRenderTargets()
{
	{
		std::array<uint32_t, eEyeType_NumEyes> indices = {}; // texture index in various swap chains
		for (uint32 eye = 0; eye < eEyeType_NumEyes; ++eye)
			indices[eye] = 0;
		m_pStereoRenderer->SetCurrentEyeSwapChainIndices(indices);
	}

	{
		// Quad layers
		std::array<uint32_t, RenderLayer::eQuadLayers_Total> indices = {}; // texture index in various swap chains
		for (uint32 i = 0; i < RenderLayer::eQuadLayers_Total; ++i)
			indices[i] = 0;
		m_pStereoRenderer->SetCurrentQuadLayerSwapChainIndices(indices);
	}
}

#include <CryRenderer/IStereoRenderer.h>
void CD3DHmdEmulatorRenderer::SubmitFrame()
{
	FUNCTION_PROFILER_RENDERER();

	CTexture* pOutput = m_pRenderer->GetActiveDisplayContext()->GetCurrentColorOutput();
	int width = pOutput->GetWidth();
	int height = pOutput->GetHeight();

	Vec4 pParams = Vec4((float)width, (float)height, 0, 0);

	static const char* CheckerboardTechniqueName = "Checkerboard";
	static const char* SideBySideTechniqueName = "SideBySide";
	static const char* LineByLineTechniqueName = "LineByLine";
	static const char* AnaglyphTechniqueName = "Anaglyph";

	const char* techName = SideBySideTechniqueName;
	auto output = m_pRenderer->GetS3DRend().GetStereoOutput();
	switch (output)
	{
	case EStereoOutput::STEREO_OUTPUT_CHECKERBOARD:        techName = CheckerboardTechniqueName; break;
	case EStereoOutput::STEREO_OUTPUT_SIDE_BY_SIDE:        techName = SideBySideTechniqueName;   break;
	case EStereoOutput::STEREO_OUTPUT_LINE_BY_LINE:        techName = LineByLineTechniqueName;   break;
	case EStereoOutput::STEREO_OUTPUT_ANAGLYPH:            techName = AnaglyphTechniqueName;     break;
	default:                                               techName = SideBySideTechniqueName;
	}

	auto stereoOutput = gcpRendD3D->GetS3DRend().GetStereoOutput();
	bool reflectConstants = stereoOutput == EStereoOutput::STEREO_OUTPUT_LINE_BY_LINE || stereoOutput == EStereoOutput::STEREO_OUTPUT_CHECKERBOARD;

	m_copyPass.SetTechnique(CShaderMan::s_ShaderStereo, techName, 0);
	m_copyPass.SetRequirePerViewConstantBuffer(false);
	m_copyPass.SetFlags(CPrimitiveRenderPass::ePassFlags_None);
	m_copyPass.SetPrimitiveFlags(reflectConstants ? CRenderPrimitive::eFlags_ReflectShaderConstants_PS : CRenderPrimitive::eFlags_None);
	m_copyPass.SetRenderTarget(0, pOutput);
	m_copyPass.SetState(GS_NODEPTHTEST);
	m_copyPass.SetTexture(0, m_scene3DRenderData[eEyeType_LeftEye].textures[0]);
	m_copyPass.SetTexture(1, m_scene3DRenderData[eEyeType_RightEye].textures[0]);
	m_copyPass.SetTexture(2, m_quadLayerRenderData[1].textures[0]);
	m_copyPass.SetTexture(3, m_quadLayerRenderData[2].textures[0]);
	m_copyPass.SetSampler(0, EDefaultSamplerStates::LinearClamp);
	m_copyPass.SetPrimitiveType(CRenderPrimitive::ePrim_ProceduralTriangle);

	{
		m_copyPass.BeginConstantUpdate();
		if (reflectConstants)
		{
			static CCryNameR screenSizeUniformName("ScreenSize");
			m_copyPass.SetConstant(screenSizeUniformName, pParams);
		}
	}

	m_copyPass.Execute();


#ifdef ENABLE_BENCHMARK_SENSOR
	const auto *leftDc = m_pStereoRenderer->GetEyeDisplayContext(CCamera::eEye_Left).first;
	const auto *rightDc = m_pStereoRenderer->GetEyeDisplayContext(CCamera::eEye_Right).first;
	if (leftDc && rightDc)
		gcpRendD3D->m_benchmarkRendererSensor->PreStereoFrameSubmit(leftDc->GetCurrentBackBuffer(), rightDc->GetCurrentBackBuffer());
#endif

#ifdef ENABLE_BENCHMARK_SENSOR
	gcpRendD3D->m_benchmarkRendererSensor->AfterStereoFrameSubmit();
#endif

	// Deactivate scene layers
	GetSceneLayerProperties(RenderLayer::eSceneLayers_0)->SetActive(false);
	for (uint32 i = 0; i < RenderLayer::eQuadLayers_Total; ++i)
		GetQuadLayerProperties(static_cast<RenderLayer::EQuadLayers>(i))->SetActive(false);
}

RenderLayer::CProperties* CD3DHmdEmulatorRenderer::GetQuadLayerProperties(RenderLayer::EQuadLayers id)
{
	if (id < RenderLayer::eQuadLayers_Total)
	{
		return &(m_layerManager.m_quadLayerProperties[id]);
	}
	return nullptr;
}

RenderLayer::CProperties* CD3DHmdEmulatorRenderer::GetSceneLayerProperties(RenderLayer::ESceneLayers id)
{
	if (id < RenderLayer::eQuadLayers_Total)
	{
		return &(m_layerManager.m_scene3DLayerProperties[id]);
	}
	return nullptr;
}

std::pair<CTexture*, Vec4> CD3DHmdEmulatorRenderer::GetMirrorTexture(EEyeType eye) const
{
	Vec4 tc = Vec4(0.0f, 0.0f, 0.5f, 1.0f);
	if (eye == eEyeType_RightEye)
		tc.x = 0.5f;

	return std::make_pair(nullptr, Vec4(0));
}

#endif // defined(INCLUDE_VR_RENDERING)