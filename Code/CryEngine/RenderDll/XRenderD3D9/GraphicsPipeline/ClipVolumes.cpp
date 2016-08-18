// Copyright 2001-2015 Crytek GmbH. All rights reserved.

#include "StdAfx.h"
#include "ClipVolumes.h"
#include "DriverD3D.h"
#include "VolumetricFog.h"

struct SPrimitiveConstants
{
	Matrix44 transformMatrix;
	Vec4     projRatioScreenScale;
	Vec4     blendPlane0;
	Vec4     blendPlane1;
};

CClipVolumesStage::CClipVolumesStage()
	: m_nShaderParamCount(0)
	, m_bClipVolumesValid(false)
	, m_bOutdoorVisible(false)
	, m_bBlendPassReady(false)
	, m_pBlendValuesRT(nullptr)
	, m_pDepthTarget(nullptr)
	, m_pClipVolumeStencilVolumeTex(nullptr)
	, m_cleared(MAX_GPU_NUM)
	, m_nearDepth(-1.0f)
	, m_raymarchDistance(0.0f)
{
	ZeroArray(m_clipVolumeShaderParams);
	memset(&m_depthTargetVolFog, 0, sizeof(m_depthTargetVolFog));
}

CClipVolumesStage::~CClipVolumesStage()
{
	SAFE_RELEASE(m_pClipVolumeStencilVolumeTex);
}

void CClipVolumesStage::Init()
{
	m_stencilPass.ClearPrimitives();
	m_blendValuesPass.ClearPrimitives();
	m_volumetricStencilPass.ClearPrimitives();

	for (int i = 0; i < MaxDeferredClipVolumes; ++i)
	{
		// Stencil pass: two primitives per clip volume, both share the same constant buffer
		{
			CConstantBufferPtr pCB = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(sizeof(SPrimitiveConstants));

			m_stencilPrimitives[2 * i + 0].SetInlineConstantBuffer(eConstantBufferShaderSlot_PerBatch, pCB, EShaderStage_Vertex);
			m_stencilPrimitives[2 * i + 1].SetInlineConstantBuffer(eConstantBufferShaderSlot_PerBatch, pCB, EShaderStage_Vertex);
		}

		// Blend values pass: one primitive per clip volume, shared CB for vertex and pixel shader
		{
			CConstantBufferPtr pCB = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(sizeof(SPrimitiveConstants));

			m_blendPrimitives[i].SetInlineConstantBuffer(eConstantBufferShaderSlot_PerBatch, pCB, EShaderStage_Pixel | EShaderStage_Vertex);
		}

	}

	// TODO: move this texture to intra-view-shared data structure after removing old graphics pipeline.
	const uint32 dsFlags = FT_NOMIPS | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_USAGE_DEPTHSTENCIL;
	CRY_ASSERT(m_pClipVolumeStencilVolumeTex == nullptr);
	m_pClipVolumeStencilVolumeTex = CTexture::CreateTextureObject("$VolFogClipVolumeStencil", 0, 0, 0, eTT_2D, dsFlags, eTF_Unknown);
}

void CClipVolumesStage::Prepare(CRenderView* pRenderView)
{
	m_nShaderParamCount = 0;
	m_bClipVolumesValid = false;
	m_bOutdoorVisible = false;

	m_pBlendValuesRT = CTexture::s_ptexVelocity;
	m_pDepthTarget = &gcpRendD3D->m_DepthBufferOrigMSAA;

	D3DViewPort viewport;
	viewport.TopLeftX = viewport.TopLeftY = 0.0f;
	viewport.Width = (float)m_pBlendValuesRT->GetWidth();
	viewport.Height = (float)m_pBlendValuesRT->GetHeight();
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	m_stencilPass.ClearPrimitives();
	m_stencilPass.SetDepthTarget(m_pDepthTarget);
	m_stencilPass.SetViewport(viewport);

	m_blendValuesPass.ClearPrimitives();
	m_blendValuesPass.SetRenderTarget(0, m_pBlendValuesRT);
	m_blendValuesPass.SetDepthTarget(m_pDepthTarget);

	m_volumetricStencilPass.ClearPrimitives();
	PrepareVolumetricFog();

	const uint32 StencilStateInitialMarkup =
	  STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
	  STENCOP_FAIL(FSS_STENCOP_KEEP) |
	  STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
	  STENCOP_PASS(FSS_STENCOP_KEEP);

	const uint32 StencilStateInsideOutsideMarkup =
	  STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
	  STENCOP_FAIL(FSS_STENCOP_KEEP) |
	  STENCOP_ZFAIL(FSS_STENCOP_INVERT) |
	  STENCOP_PASS(FSS_STENCOP_KEEP);

	const uint32 StencilStateTest =
	  STENC_FUNC(FSS_STENCFUNC_EQUAL) |
	  STENCOP_FAIL(FSS_STENCOP_KEEP) |
	  STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
	  STENCOP_PASS(FSS_STENCOP_KEEP);

	const uint8 StencilReadWriteMask = 0xFF & ~BIT_STENCIL_RESERVED;

	static CCryNameTSCRC techStencil = "ClipVolumeStencil";
	static CCryNameTSCRC techPortalBlend = "ClipVolumeBlendValue";

	static ICVar* pPortalsBlendCVar = iConsole->GetCVar("e_PortalsBlend");
	const bool bRenderPortalBlendValues = pPortalsBlendCVar->GetIVal() > 0;
	const bool bRenderVisAreas = CRenderer::CV_r_VisAreaClipLightsPerPixel > 0;

	const auto& clipVolumes = pRenderView->GetClipVolumes();
	CStandardGraphicsPipeline::SViewInfo viewInfo[2];
	int viewInfoCount = gcpRendD3D->GetGraphicsPipeline().GetViewInfo(viewInfo);
	const bool bReverseDepth = (viewInfo[0].flags & CStandardGraphicsPipeline::SViewInfo::eFlags_ReverseDepth) != 0;

	Vec2 projRatio;
	float zn = viewInfo[0].pRenderCamera->fNear;
	float zf = viewInfo[0].pRenderCamera->fFar;
	projRatio.x = bReverseDepth ? zn / (zn - zf) : zf / (zf - zn);
	projRatio.y = bReverseDepth ? zn / (zf - zn) : zn / (zn - zf);

	m_nShaderParamCount = 0;
	for (int i = 0; i < VisAreasOutdoorStencilOffset; ++i)
	{
		uint32 nFlags = IClipVolume::eClipVolumeConnectedToOutdoor | IClipVolume::eClipVolumeAffectedBySun;
		m_clipVolumeShaderParams[m_nShaderParamCount++] = Vec4(gEnv->p3DEngine->GetSkyColor() * gcpRendD3D->m_fAdaptedSceneScaleLBuffer, alias_cast<float>(nFlags));
	}

	for (const auto& volume : clipVolumes)
	{
		// Update shader params
		const uint32 paramIndex = volume.nStencilRef + 1;
		CRY_ASSERT(paramIndex >= VisAreasOutdoorStencilOffset);

		if (paramIndex < MaxDeferredClipVolumes)
		{
			uint32 nData =
			  (volume.blendInfo[1].blendID + 1) << 24 |
			  (volume.blendInfo[0].blendID + 1) << 16 |
			  volume.nFlags;

			m_clipVolumeShaderParams[paramIndex] = Vec4(0, 0, 0, alias_cast<float>(nData));
			m_bOutdoorVisible |= volume.nFlags & IClipVolume::eClipVolumeConnectedToOutdoor ? true : false;

			m_nShaderParamCount = max(m_nShaderParamCount, paramIndex + 1);
		}
	}

	// Create stencil and blend pass primitives
	for (int i = clipVolumes.size() - 1; i >= 0; --i)
	{
		const SDeferredClipVolume& volume = clipVolumes[i];
		if (volume.m_pRenderMesh && volume.nStencilRef < MaxDeferredClipVolumes)
		{
			if ((volume.nFlags & IClipVolume::eClipVolumeIsVisArea) != 0 && !bRenderVisAreas)
				continue;

			CRY_ASSERT(((volume.nStencilRef + 1) & (BIT_STENCIL_RESERVED | BIT_STENCIL_INSIDE_CLIPVOLUME)) == 0);
			const int stencilRef = ~(volume.nStencilRef + 1) & ~(BIT_STENCIL_RESERVED | BIT_STENCIL_INSIDE_CLIPVOLUME);

			buffer_handle_t hVertexStream = ~0u;
			buffer_handle_t hIndexStream = ~0u;
			CRenderMesh* pRenderMesh = nullptr;

			if (pRenderMesh = reinterpret_cast<CRenderMesh*>(volume.m_pRenderMesh.get()))
			{
				pRenderMesh->CheckUpdate(pRenderMesh->_GetVertexFormat(), 0);

				hVertexStream = pRenderMesh->_GetVBStream(VSF_GENERAL);
				hIndexStream = pRenderMesh->_GetIBStream();

				if (hVertexStream != ~0u && hIndexStream != ~0u)
				{
					// Initial markup
					CRenderPrimitive& primInit = m_stencilPrimitives[2 * i + 0];
					primInit.SetTechnique(CShaderMan::s_shDeferredShading, techStencil, 0);
					primInit.SetRenderState((bReverseDepth ? GS_DEPTHFUNC_GEQUAL : GS_DEPTHFUNC_LEQUAL) | GS_STENCIL);
					primInit.SetCustomVertexStream(hVertexStream, pRenderMesh->_GetVertexFormat(), pRenderMesh->GetStreamStride(VSF_GENERAL));
					primInit.SetCustomIndexStream(hIndexStream, (sizeof(vtx_idx) == 2 ? Index16 : Index32));
					primInit.SetCullMode(eCULL_Front);
					primInit.SetStencilState(StencilStateInitialMarkup, stencilRef, StencilReadWriteMask, StencilReadWriteMask);
					primInit.SetDrawInfo(eptTriangleList, 0, 0, pRenderMesh->_GetNumInds());

					// Update constant buffer. NOTE: buffer is assigned to preallocated primitives
					auto& constantManager = primInit.GetConstantManager();

					auto constants = constantManager.BeginTypedConstantUpdate<SPrimitiveConstants>(eConstantBufferShaderSlot_PerBatch, EShaderStage_Vertex);
					constants->transformMatrix = Matrix44(volume.mWorldTM.GetTransposed()) * viewInfo[0].cameraProjMatrix;
					
					if (viewInfoCount>1)
					{
						constants.BeginStereoOverride();
						constants->transformMatrix = Matrix44(volume.mWorldTM.GetTransposed()) * viewInfo[1].cameraProjMatrix;
					}

					constantManager.EndTypedConstantUpdate(constants);

					m_stencilPass.AddPrimitive(&primInit);
					m_volumetricStencilPass.AddPrimitive(&primInit);

					// Inside/outside test
					CRenderPrimitive& primMarkup = m_stencilPrimitives[2 * i + 1];
					primMarkup.SetTechnique(CShaderMan::s_shDeferredShading, techStencil, 0);
					primMarkup.SetRenderState((bReverseDepth ? GS_DEPTHFUNC_GEQUAL : GS_DEPTHFUNC_LEQUAL) | GS_STENCIL);
					primMarkup.SetCustomVertexStream(hVertexStream, pRenderMesh->_GetVertexFormat(), pRenderMesh->GetStreamStride(VSF_GENERAL));
					primMarkup.SetCustomIndexStream(hIndexStream, (sizeof(vtx_idx) == 2 ? Index16 : Index32));
					primMarkup.SetCullMode(eCULL_None);
					primMarkup.SetStencilState(StencilStateInsideOutsideMarkup, ~stencilRef, StencilReadWriteMask, StencilReadWriteMask);
					primMarkup.SetDrawInfo(eptTriangleList, 0, 0, pRenderMesh->_GetNumInds());
					m_stencilPass.AddPrimitive(&primMarkup);
					m_volumetricStencilPass.AddPrimitive(&primMarkup);
				}
			}

			// Blend values
			if (bRenderPortalBlendValues && (volume.nFlags & IClipVolume::eClipVolumeBlend))
			{
				const int stencilTestRef = BIT_STENCIL_INSIDE_CLIPVOLUME + volume.nStencilRef + 1;

				CRenderPrimitive& primBlend = m_blendPrimitives[i];
				primBlend.SetTechnique(CShaderMan::s_shDeferredShading, techPortalBlend, 0);
				primBlend.SetRenderState(GS_STENCIL | GS_NODEPTHTEST | GS_NOCOLMASK_R | GS_NOCOLMASK_B | GS_NOCOLMASK_A);
				primBlend.SetTexture(3, CTexture::s_ptexZTarget);
				primBlend.SetCullMode(eCULL_Front);
				primBlend.SetStencilState(StencilStateTest, stencilTestRef, StencilReadWriteMask, StencilReadWriteMask);

				auto& constantManager = primBlend.GetConstantManager();
				auto constants = constantManager.BeginTypedConstantUpdate<SPrimitiveConstants>(eConstantBufferShaderSlot_PerBatch, EShaderStage_Pixel | EShaderStage_Vertex);

				constants->projRatioScreenScale = Vec4(projRatio.x, projRatio.y, 1.0f / m_pBlendValuesRT->GetWidth(), 1.0f / m_pBlendValuesRT->GetHeight());
				constants->blendPlane0 = viewInfo[0].invCameraProjMatrix * volume.blendInfo[0].blendPlane;
				constants->blendPlane1 = viewInfo[0].invCameraProjMatrix * volume.blendInfo[1].blendPlane;

				if (hVertexStream != ~0u && hIndexStream != ~0u)
				{
					constants->transformMatrix = Matrix44(volume.mWorldTM.GetTransposed()) * viewInfo[0].cameraProjMatrix;

					primBlend.SetCustomVertexStream(hVertexStream, pRenderMesh->_GetVertexFormat(), pRenderMesh->GetStreamStride(VSF_GENERAL));
					primBlend.SetCustomIndexStream(hIndexStream, (sizeof(vtx_idx) == 2 ? Index16 : Index32));
					primBlend.SetDrawInfo(eptTriangleList, 0, 0, pRenderMesh->_GetNumInds());
				}
				else
				{
					constants->transformMatrix = Matrix44(IDENTITY);

					primBlend.SetPrimitiveType(CRenderPrimitive::ePrim_Triangle);
					primBlend.SetDrawInfo(eptTriangleList, 0, 0, 3);
				}

				if (viewInfoCount > 1)
				{
					constants.BeginStereoOverride();
					constants->blendPlane0 = viewInfo[1].invCameraProjMatrix * volume.blendInfo[0].blendPlane;
					constants->blendPlane1 = viewInfo[1].invCameraProjMatrix * volume.blendInfo[1].blendPlane;

					if (hVertexStream != ~0u && hIndexStream != ~0u)
						constants->transformMatrix = Matrix44(volume.mWorldTM.GetTransposed()) * viewInfo[1].cameraProjMatrix;

				}

				constantManager.EndTypedConstantUpdate(constants);

				m_blendValuesPass.AddPrimitive(&primBlend);
			}
		}
	}
}

void CClipVolumesStage::Execute()
{
	PROFILE_LABEL_SCOPE("CLIPVOLUMES");

	D3DViewPort viewport;
	viewport.TopLeftX = viewport.TopLeftY = 0.0f;
	viewport.Width = (float)m_pBlendValuesRT->GetWidth();
	viewport.Height = (float)m_pBlendValuesRT->GetHeight();
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Render clip volumes to stencil
	{
		m_stencilPass.SetDepthTarget(m_pDepthTarget);
		m_stencilPass.SetViewport(viewport);
		m_stencilPass.Execute();
	}

	// Render portal blend values
	{
		m_blendValuesPass.SetRenderTarget(0, m_pBlendValuesRT);
		m_blendValuesPass.SetDepthTarget(m_pDepthTarget);
		m_blendValuesPass.SetViewport(viewport);
		m_blendValuesPass.Execute();
	}

	// Resolve final stencil buffer
	{
		static CCryNameTSCRC techResolveStencil("ResolveStencil");

		auto stencilOnlyView = SResourceView::ShaderResourceView(m_pDepthTarget->pTexture->GetDstFormat(),
		                                                         0, -1, 0, -1, false, gcpRendD3D->FX_GetMSAAMode() > 0, SResourceView::eSRV_StencilOnly);

		m_stencilResolvePass.SetTechnique(CShaderMan::s_shDeferredShading, techResolveStencil, 0);
		m_stencilResolvePass.SetRenderTarget(0, m_pBlendValuesRT);
		m_stencilResolvePass.SetState(GS_NODEPTHTEST | GS_NOCOLMASK_G | GS_NOCOLMASK_B | GS_NOCOLMASK_A);
		m_stencilResolvePass.SetTexture(4, m_pDepthTarget->pTexture, stencilOnlyView.m_Desc.Key);
		m_stencilResolvePass.BeginConstantUpdate();
		m_stencilResolvePass.Execute();
	}

	ExecuteVolumetricFog();

	m_bClipVolumesValid = true;
}

CTexture* CClipVolumesStage::GetClipVolumeStencilVolumeTexture() const
{
	return m_pClipVolumeStencilVolumeTex;
}

void CClipVolumesStage::PrepareVolumetricFog()
{
	if (gcpRendD3D->m_bVolumetricFogEnabled)
	{
		const int32 screenWidth = gcpRendD3D->GetWidth();
		const int32 screenHeight = gcpRendD3D->GetHeight();

		// size for view frustum aligned volume texture.
		const int32 scaledWidth = CVolumetricFogStage::GetVolumeTextureSize(screenWidth, CRenderer::CV_r_VolumetricFogTexScale);
		const int32 scaledHeight = CVolumetricFogStage::GetVolumeTextureSize(screenHeight, CRenderer::CV_r_VolumetricFogTexScale);
		const int32 depth = CVolumetricFogStage::GetVolumeTextureDepthSize();

		D3DViewPort viewport;
		viewport.TopLeftX = viewport.TopLeftY = 0.0f;
		viewport.Width = static_cast<float>(scaledWidth);
		viewport.Height = static_cast<float>(scaledHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		m_passWriteJitteredDepth.SetViewport(viewport);
		m_volumetricStencilPass.SetViewport(viewport);

		CRY_ASSERT(m_pClipVolumeStencilVolumeTex);
		if (m_pClipVolumeStencilVolumeTex
		    && (m_pClipVolumeStencilVolumeTex->GetWidth() != scaledWidth
		        || m_pClipVolumeStencilVolumeTex->GetHeight() != scaledHeight
		        || !CTexture::IsTextureExist(m_pClipVolumeStencilVolumeTex))
		    || m_depthTargetArrayVolFog.empty()
		    || m_depthTargetArrayVolFog.size() != depth)
		{
			m_depthTargetArrayVolFog.clear();
			m_depthTargetArrayVolFog.resize(depth);

			const uint32 commonFlags = FT_NOMIPS | FT_DONT_STREAM;
			const uint32 rtFlags = commonFlags | FT_USAGE_RENDERTARGET;
			const uint32 dsFlags = rtFlags | FT_USAGE_DEPTHSTENCIL | FT_DONT_RELEASE;
			const int32 w = scaledWidth;
			const int32 h = scaledHeight;
			const int32 d = depth;
			ETEX_Format format = eTF_D24S8;
			CTexture* pTex = CTexture::CreateTextureArray("$VolFogClipVolumeStencil", eTT_2D, w, h, d, 1, dsFlags, format);

			if (pTex == nullptr
			    || m_pClipVolumeStencilVolumeTex != pTex // texture name must exactly match the name when creating texture object.
			    || pTex->GetFlags() & FT_FAILED)
			{
				CryFatalError("Couldn't allocate texture.");
			}

			auto pStencilReadSRV = m_pClipVolumeStencilVolumeTex->GetDeviceStencilReadOnlySRV(0, d);
			m_pClipVolumeStencilVolumeTex->SetShaderResourceView(pStencilReadSRV, false);

			// DSV to clear stencil buffer.
			m_depthTargetVolFog.nWidth = m_pClipVolumeStencilVolumeTex->GetWidth();
			m_depthTargetVolFog.nHeight = m_pClipVolumeStencilVolumeTex->GetHeight();
			m_depthTargetVolFog.pTexture = m_pClipVolumeStencilVolumeTex;
			m_depthTargetVolFog.pTarget = m_pClipVolumeStencilVolumeTex->GetDevTexture()->Get2DTexture();
			m_depthTargetVolFog.pSurface = m_pClipVolumeStencilVolumeTex->GetDeviceDepthStencilView();

			// clip volumes are rendered to separate DSVs because the GPU performance is faster than single DSV.
			for (int32 i = 0; i < m_depthTargetArrayVolFog.size(); ++i)
			{
				auto& depthTarget = m_depthTargetArrayVolFog[i];
				depthTarget = m_depthTargetVolFog;
				depthTarget.pSurface = m_pClipVolumeStencilVolumeTex->GetDeviceDepthStencilView(i, 1); // separate DSV
			}

			if(!m_depthTargetArrayVolFog.empty())
			{
				// set depth target before AddPrimitive() is called.
				auto& depthTarget = m_depthTargetArrayVolFog[0];
				m_volumetricStencilPass.SetDepthTarget(&depthTarget);
			}

			// this buffer is managed explicitly on multi GPU.
			m_pClipVolumeStencilVolumeTex->DisableMgpuSync();

			// need to clear depth buffer.
			const int32 safeMargin = 2; // workaround: shader isn't activated in initial several frames.
			m_cleared = gcpRendD3D->GetActiveGPUCount() + safeMargin;
		}
	}
}

void CClipVolumesStage::ExecuteVolumetricFog()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	SRenderPipeline& rp(rd->m_RP);
	auto nThreadID = rp.m_nProcessThreadID;

	if (rd->m_bVolumetricFogEnabled && rp.m_TI[nThreadID].m_FS.m_bEnable)
	{
		PROFILE_LABEL_SCOPE("CLIPVOLUMES FOR VOLUMETRIC FOG");

		const bool bReverseDepth = (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
		const float nearDepth = bReverseDepth ? 1.0f : 0.0f;
		const float raymarchDistance = gRenDev->m_cEF.m_PF[rp.m_nProcessThreadID].pVolumetricFogDistanceParams.w;

		if (m_nearDepth != nearDepth
		    || (0.25f < abs(m_raymarchDistance - raymarchDistance)))
		{
			// store values to observe the change.
			m_nearDepth = nearDepth;
			m_raymarchDistance = raymarchDistance;

			// need to clear depth buffer.
			m_cleared = (m_cleared > 0) ? m_cleared : rd->GetActiveGPUCount();
		}

		auto maxDSVCount = m_depthTargetArrayVolFog.size();
		const float fFar = rd->GetRCamera().fFar;
		const float fNear = rd->GetRCamera().fNear;
		const float factor0 = fFar / (fFar - fNear);
		const float factor1 = fNear * -factor0;

		if (m_cleared > 0)
		{
			// clear depth when it's needed.
			rd->FX_ClearTarget(m_depthTargetVolFog.pSurface, CLEAR_STENCIL | CLEAR_ZBUFFER, 0.0f, 0);
		}
		else
		{
			rd->FX_ClearTarget(m_depthTargetVolFog.pSurface, CLEAR_STENCIL, 0.0f, 0);
		}

		// Render clip volumes to stencil array.
		for (int32 i = 0; i < maxDSVCount; ++i)
		{
			// separate DSV
			auto& depthTarget = m_depthTargetArrayVolFog[i];

			if (m_cleared > 0)
			{
				// write jittering depth when it's needed.
				static CCryNameTSCRC shaderName("StoreJitteringDepthToClipVolumeDepth");
				m_passWriteJitteredDepth.SetTechnique(CShaderMan::s_shDeferredShading, shaderName, 0);
				m_passWriteJitteredDepth.SetState(GS_DEPTHWRITE | GS_DEPTHFUNC_NOTEQUAL);
				m_passWriteJitteredDepth.SetDepthTarget(&depthTarget);

				m_passWriteJitteredDepth.BeginConstantUpdate();

				static CCryNameR paramDepth("ParamDepth");
				const Vec4 vParamDepth(static_cast<float>(i), factor1, factor0, nearDepth);
				m_passWriteJitteredDepth.SetConstant(eHWSC_Pixel, paramDepth, vParamDepth);

				m_passWriteJitteredDepth.Execute();
			}

			m_volumetricStencilPass.SetDepthTarget(&depthTarget);
			m_volumetricStencilPass.Execute();
		}

		m_cleared = (m_cleared > 0) ? (m_cleared - 1) : 0;
	}
}
