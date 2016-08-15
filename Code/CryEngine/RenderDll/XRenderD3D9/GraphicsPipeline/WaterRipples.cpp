// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "WaterRipples.h"

#include "DriverD3D.h"
#include "D3DPostProcess.h"

//////////////////////////////////////////////////////////////////////////

namespace
{
void CopyTextureInternal(CTexture* pSrcTex, CTexture* pDestTex)
{
	CRY_ASSERT(pDestTex);
	CRY_ASSERT(pSrcTex);
	D3D11_BOX copyRegion;
	ZeroStruct(copyRegion);
	copyRegion.back = 1;
	copyRegion.right = pDestTex->GetWidth();
	copyRegion.bottom = pDestTex->GetHeight();
	auto* pDestDevTex = pDestTex->GetDevTexture()->GetBaseTexture();
	auto* pSrcDevTex = pSrcTex->GetDevTexture()->GetBaseTexture();
	gcpRendD3D->GetDeviceContext().CopySubresourceRegion(pDestDevTex, 0, 0, 0, 0, pSrcDevTex, 0, &copyRegion);
}
}

CWaterRipplesStage::CWaterRipplesStage()
	: m_vertexBuffer(~0u)
	, m_pTempTexture(nullptr)
	, m_pCVarWaterRipplesDebug(nullptr)
	, m_ripplesGenTechName("WaterRipplesGen")
	, m_ripplesHitTechName("WaterRipplesHit")
	, m_ripplesParamName("WaterRipplesParams")
	, m_frameID(-1)
	, m_samplerLinearClamp(-1)
	, m_lastSpawnTime(0.0f)
	, m_lastUpdateTime(0.0f)
	, m_simGridSnapRange(5.0f)
	, m_simOrigin(0.0f)
	, m_updateMask(0)
	, m_shaderParam(0.0f)
	, m_lookupParam(0.0f)
	, m_bInitializeSim(true)
	, m_bSnapToCenter(false)
{
}

CWaterRipplesStage::~CWaterRipplesStage()
{
	if (m_vertexBuffer != ~0u)
	{
		gRenDev->m_DevBufMan.Destroy(m_vertexBuffer);
	}

	SAFE_RELEASE(m_pTempTexture);
}

void CWaterRipplesStage::Init()
{
	CRY_ASSERT(m_vertexBuffer == ~0u);
	m_vertexBuffer = gRenDev->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, sTotalVertexCount * sVertexStride);

	CRY_ASSERT(m_pTempTexture == nullptr);
	const uint32 flags = FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_NOMIPS;
	m_pTempTexture = CTexture::CreateTextureObject("$WaterRippleGenTemp", 0, 0, 0, eTT_2D, flags, eTF_Unknown);

	m_samplerLinearClamp = CTexture::GetTexState(STexState(FILTER_LINEAR, TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP, 0x0));

	int32 vertexOffset = 0;
	for (auto& prim : m_ripplePrimitive)
	{
		prim.SetFlags(CRenderPrimitive::eFlags_ReflectConstantBuffersFromShader);
		prim.SetCullMode(eCULL_None);
		prim.SetCustomVertexStream(m_vertexBuffer, sVertexFormat, sVertexStride);
		prim.SetCustomIndexStream(~0u, 0);
		prim.SetDrawInfo(eptTriangleStrip, 0, vertexOffset, sVertexCount);

		// only update blue channel: current frame
		prim.SetRenderState(GS_BLSRC_ONE | GS_BLDST_ONE | GS_NODEPTHTEST | GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_A);

		vertexOffset += sVertexCount;
	}
}

void CWaterRipplesStage::Prepare(CRenderView* pRenderView)
{
	CRY_ASSERT(pRenderView);

	if (IsVisible(pRenderView) && CTexture::IsTextureExist(CTexture::s_ptexWaterRipplesDDN))
	{
		const int32 width = CTexture::s_ptexWaterRipplesDDN->GetWidth();
		const int32 height = CTexture::s_ptexWaterRipplesDDN->GetHeight();
		const auto format = CTexture::s_ptexWaterRipplesDDN->GetTextureDstFormat();
		const uint32 flags = FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_NOMIPS;

		if (m_pTempTexture != nullptr
		    && (!CTexture::IsTextureExist(m_pTempTexture)
		        || m_pTempTexture->Invalidate(width, height, format)))
		{
			if (!m_pTempTexture->Create2DTexture(width, height, 1, flags, nullptr, format, format))
			{
				CryFatalError("Couldn't allocate texture.");
			}
		}
	}

	if (!m_pCVarWaterRipplesDebug)
	{
		m_pCVarWaterRipplesDebug = gEnv->pConsole->GetCVar("e_WaterRipplesDebug");
	}
}

bool CWaterRipplesStage::Update(CRenderView* pRenderView)
{
	CRY_ASSERT(pRenderView);

	const bool bAlreadyUpdated = (m_frameID == gRenDev->GetFrameID());
	if (bAlreadyUpdated)
	{
		return false;
	}
	m_frameID = gRenDev->GetFrameID();

	const auto& waterRipples = pRenderView->GetWaterRipples();
	for (auto& ripple : waterRipples)
	{
		if (m_waterRipples.size() < SWaterRippleInfo::MaxWaterRipplesInScene)
		{
			m_waterRipples.emplace_back(ripple);
		}
	}

#if !defined(_RELEASE)
	UpdateAndDrawDebugInfo(pRenderView);
#endif

	const uint32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;
	const float fTime = gRenDev->m_RP.m_TI[nThreadID].m_RealTime;
	const uint32 gpuCount = gRenDev->GetActiveGPUCount();

	const float simGridSize = static_cast<float>(SWaterRippleInfo::WaveSimulationGridSize);
	const float halfSimGridSize = 0.5f * simGridSize;

	// always snap by entire pixels to avoid errors when displacing the simulation
	const float fPixelSizeWS =
	  (CTexture::s_ptexWaterRipplesDDN->GetWidth() > 0)
	  ? (simGridSize / CTexture::s_ptexWaterRipplesDDN->GetWidth())
	  : 1.0f;
	m_simGridSnapRange = max(ceilf(m_simGridSnapRange / fPixelSizeWS), 1.f) * fPixelSizeWS;

	// only allow updates every 25ms - for lower frames, would need to iterate multiple times
	if (gpuCount == 1)
	{
		if (fTime - m_lastUpdateTime < 0.025f)
		{
			return false;
		}

		m_lastUpdateTime = fTime - fmodf(fTime - m_lastUpdateTime, 0.025f);
	}
	else
	{
		if (fTime - m_lastUpdateTime <= 0.0f)
		{
			return false;
		}

		m_lastUpdateTime = gRenDev->m_RP.m_TI[nThreadID].m_RealTime;
	}

	Vec4 vParams = Vec4(0, 0, 0, 0);
	if (!m_waterRipples.empty())
	{
		vParams = Vec4(m_waterRipples[0].position.x, m_waterRipples[0].position.y, 0.0f, 1.0f);
		m_lastSpawnTime = fTime;
	}

	if (!m_updateMask)
	{
		const Vec3& cameraPos = gRenDev->GetRCamera().vOrigin;
		const float xsnap = ceilf(cameraPos.x / m_simGridSnapRange) * m_simGridSnapRange;
		const float ysnap = ceilf(cameraPos.y / m_simGridSnapRange) * m_simGridSnapRange;

		m_bSnapToCenter = false;
		if (m_simOrigin.x != xsnap || m_simOrigin.y != ysnap)
		{
			m_bSnapToCenter = true;
			vParams.x = (xsnap - m_simOrigin.x) * m_lookupParam.x;
			vParams.y = (ysnap - m_simOrigin.y) * m_lookupParam.x;

			m_simOrigin = Vec2(xsnap, ysnap);
		}

		m_shaderParam = vParams;
		m_waterRipplesMGPU = m_waterRipples;
		m_waterRipples.clear();

		// update world space -> sim space transform
		m_lookupParam.x = 1.f / simGridSize;
		m_lookupParam.y = halfSimGridSize - m_simGridSnapRange;
		m_lookupParam.z = -m_simOrigin.x * m_lookupParam.x + 0.5f;
		m_lookupParam.w = -m_simOrigin.y * m_lookupParam.x + 0.5f;

		if (gpuCount > 1)
		{
			m_updateMask = (1 << gpuCount) - 1;
		}
	}

	return true;
}

void CWaterRipplesStage::Execute(CRenderView* pRenderView)
{
	CRY_ASSERT(pRenderView);

	if (!IsVisible(pRenderView))
	{
		m_bInitializeSim = true;

		// don't process water ripples.
		return;
	}

	if (!CTexture::IsTextureExist(CTexture::s_ptexWaterRipplesDDN)
	    //|| !CTexture::IsTextureExist(CTexture::s_ptexBackBufferScaled[0]))
	    || !CTexture::IsTextureExist(m_pTempTexture))
	{
		return;
	}

	if (!Update(pRenderView))
	{
		return;
	}

	{
		PROFILE_LABEL_SCOPE("WATER RIPPLES GEN");

		// TODO: fix to use shared temporary texture.
		//auto* pTempTex = CTexture::s_ptexBackBufferScaled[0];
		auto* pTempTex = m_pTempTexture;

		// Initialize sim on first frame
		if (m_bInitializeSim)
		{
			m_bInitializeSim = false;

			const RECT rect = { 0, 0, pTempTex->GetWidth(), pTempTex->GetHeight() };
			gcpRendD3D->FX_ClearTarget(pTempTex, Clr_Transparent, 1, &rect, true);
		}

		D3DViewPort viewport;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = static_cast<float>(CTexture::s_ptexWaterRipplesDDN->GetWidth());
		viewport.Height = static_cast<float>(CTexture::s_ptexWaterRipplesDDN->GetHeight());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// Snapping occurred - update simulation to new offset
		if (m_bSnapToCenter)
		{
			auto& pass = m_passSnapToCenter;

			if (pass.InputChanged())
			{
				uint64 rtMask = g_HWSR_MaskBit[HWSR_SAMPLE0];

				pass.SetTechnique(CShaderMan::s_shPostEffects, m_ripplesGenTechName, rtMask);

				pass.SetTexture(0, CTexture::s_ptexWaterRipplesDDN);
				pass.SetSampler(0, m_samplerLinearClamp);

				pass.SetRenderTarget(0, pTempTex);
				pass.SetViewport(viewport);

				pass.SetState(GS_NODEPTHTEST);
			}

			pass.BeginConstantUpdate();

			pass.SetConstant(eHWSC_Pixel, m_ripplesParamName, m_shaderParam);

			pass.Execute();

			CopyTextureInternal(pTempTex, CTexture::s_ptexWaterRipplesDDN);
		}

		// Compute wave propagation
		{
			auto& pass = m_passWaterWavePropagation;

			if (pass.InputChanged())
			{
				pass.SetTechnique(CShaderMan::s_shPostEffects, m_ripplesGenTechName, 0);

				pass.SetTexture(0, CTexture::s_ptexWaterRipplesDDN);
				pass.SetSampler(0, m_samplerLinearClamp);

				pass.SetRenderTarget(0, pTempTex);
				pass.SetViewport(viewport);

				pass.SetState(GS_NODEPTHTEST);
			}

			pass.BeginConstantUpdate();

			pass.SetConstant(eHWSC_Pixel, m_ripplesParamName, m_shaderParam);

			pass.Execute();
		}

		ExecuteWaterRipples(pRenderView, pTempTex, viewport);

		CopyTextureInternal(pTempTex, CTexture::s_ptexWaterRipplesDDN);

		m_passMipmapGen.Execute(CTexture::s_ptexWaterRipplesDDN);
	}

	m_updateMask &= ~(1 << gRenDev->RT_GetCurrGpuID());
}

bool CWaterRipplesStage::IsVisible(CRenderView* pRenderView) const
{
	const float fTimeOut = 10.0f; // 10 seconds
	const bool bSimTimeOut = (gEnv->pTimer->GetCurrTime() - m_lastSpawnTime) > fTimeOut;

	auto& waterRipples = pRenderView->GetWaterRipples();
	const bool bEmpty = waterRipples.empty()
	                    && m_waterRipples.empty();

	bool bRunnable = (m_updateMask
	                  || ((CRenderer::CV_r_PostProcessGameFx != 0)
	                      && (!bEmpty || !bSimTimeOut)));

	return bRunnable;
}

void CWaterRipplesStage::ExecuteWaterRipples(CRenderView* pRenderView, CTexture* pTargetTex, const D3DViewPort& viewport)
{
	CRY_ASSERT(pRenderView);

	auto& waterRipples = m_waterRipplesMGPU;
	if (waterRipples.empty())
	{
		return;
	}

	const float z = (gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 1.0f : 0.0f;

	const float fWidth = viewport.Width;
	const float fHeight = viewport.Height;
	const float fWidthRcp = 1.0f / fWidth;
	const float fHeightRcp = 1.0f / fHeight;
	const float fRatio = fHeightRcp / fWidthRcp;

	const float fWidthHit = 2.0f * fWidthRcp * fRatio;
	const float fHeightHit = 2.0f * fHeightRcp;

	gcpRendD3D->Set2DMode(true, static_cast<int32>(viewport.Width), (int32)viewport.Height);

	{
		auto& pass = m_passAddWaterRipples;
		pass.ClearPrimitives();
		pass.SetRenderTarget(0, pTargetTex);
		pass.SetViewport(viewport);

		Vec4 param = m_shaderParam;
		int32 vertexOffset = 0;

		CRY_ASSERT(waterRipples.size() <= SWaterRippleInfo::MaxWaterRipplesInScene);
		CryStackAllocWithSizeVector(SVertex, sTotalVertexCount, ripplesVertices, CDeviceBufferManager::AlignBufferSizeForStreaming);

		for (int32 i = 0; i < waterRipples.size(); ++i)
		{
			const auto& ripple = waterRipples[i];

			// Map hit world space to simulation space
			float xmapped = ripple.position.x * m_lookupParam.x + m_lookupParam.z;
			float ymapped = ripple.position.y * m_lookupParam.x + m_lookupParam.w;

			// Render a sprite at hit location
			float x0 = (xmapped - 0.5f * (fWidthHit * ripple.scale)) * fWidth;
			float y0 = (ymapped - 0.5f * (fHeightHit * ripple.scale)) * fHeight;

			float x1 = (xmapped + 0.5f * (fWidthHit * ripple.scale)) * fWidth;
			float y1 = (ymapped + 0.5f * (fHeightHit * ripple.scale)) * fHeight;

			// sprite vertices for a water ripple
			ripplesVertices[vertexOffset++] = {
				Vec3(x0, y0, z),
				{
					{ 0 }
				},
				Vec2(0,  0)
			};
			ripplesVertices[vertexOffset++] = {
				Vec3(x0, y1, z),
				{
					{ 0 }
				},
				Vec2(0,  1)
			};
			ripplesVertices[vertexOffset++] = {
				Vec3(x1, y0, z),
				{
					{ 0 }
				},
				Vec2(1,  0)
			};
			ripplesVertices[vertexOffset++] = {
				Vec3(x1, y1, z),
				{
					{ 0 }
				},
				Vec2(1,  1)
			};

			auto& primitive = m_ripplePrimitive[i];

			primitive.SetTechnique(CShaderMan::s_shPostEffects, m_ripplesHitTechName, 0);

			// update constant buffer
			{
				uint64 prevRTMask = gcpRendD3D->m_RP.m_FlagsShader_RT;
				gcpRendD3D->m_RP.m_FlagsShader_RT = primitive.GetShaderRtMask();
				primitive.GetConstantManager().BeginNamedConstantUpdate();

				// Pass height scale to shader
				param.w = ripple.strength;
				primitive.GetConstantManager().SetNamedConstant(eHWSC_Pixel, m_ripplesParamName, param);

				// Engine viewport needs to be set so that data is available when filling reflected PB constants
				gcpRendD3D->RT_SetViewport((int32)viewport.TopLeftX, (int32)viewport.TopLeftY, (int32)viewport.Width, (int32)viewport.Height);

				primitive.GetConstantManager().EndNamedConstantUpdate(); // Unmap constant buffers and mark as bound
				gcpRendD3D->m_RP.m_FlagsShader_RT = prevRTMask;
			}

			pass.AddPrimitive(&primitive);
		}

		// update all sprite vertices
		CRY_ASSERT(vertexOffset > 0);
		gRenDev->m_DevBufMan.UpdateBuffer(m_vertexBuffer, ripplesVertices, vertexOffset * sVertexStride);

		pass.Execute();
	}

	gcpRendD3D->Set2DMode(false, (int32)viewport.Width, (int32)viewport.Height);
}

void CWaterRipplesStage::UpdateAndDrawDebugInfo(CRenderView* pRenderView)
{
#if !defined(_RELEASE)
	const float maxLifetime = 2.0f;
	const float frameTime = std::max(0.0f, gEnv->pTimer->GetFrameTime());

	// process lifetime.
	for (auto& debugInfo : m_debugRippleInfos)
	{
		if (debugInfo.lifetime > 0.0f)
		{
			debugInfo.lifetime -= frameTime;
		}
	}

	const auto& waterRipples = pRenderView->GetWaterRipples();
	const auto newRippleDebugCount = waterRipples.size();
	const auto remainingRippleDebugCount = std::count_if(m_debugRippleInfos.begin(), m_debugRippleInfos.end(),
	                                                     [](const SWaterRippleRecord& record)
	{
		return (record.lifetime > 0.0f);
	});
	const auto deadRippleDebugCount = m_debugRippleInfos.size() - remainingRippleDebugCount;
	const auto maxRippleDebugCount = 2 * SWaterRippleInfo::MaxWaterRipplesInScene; // assign enough size to hold debug infos.

	// remove old ripple debug infos to make empty spaces for newly added ripples.
	auto rippleDebugCount = newRippleDebugCount + remainingRippleDebugCount;
	if (rippleDebugCount > maxRippleDebugCount)
	{
		std::sort(m_debugRippleInfos.begin(), m_debugRippleInfos.end(),
		          [](const SWaterRippleRecord& left, const SWaterRippleRecord& right)
		{
			return (left.lifetime < right.lifetime);
		});

		const auto removedCount = rippleDebugCount - maxRippleDebugCount;
		auto removeBegin = std::next(m_debugRippleInfos.begin(), deadRippleDebugCount); // old infos exist after dead infos.
		for_each(removeBegin, std::next(removeBegin, removedCount),
		         [](SWaterRippleRecord& record)
		{
			record.lifetime = 0.0f;
		});
	}

	// add new ripple debug infos.
	for (auto& info : waterRipples)
	{
		auto element = std::find_if(m_debugRippleInfos.begin(), m_debugRippleInfos.end(),
		                            [](const SWaterRippleRecord& record)
		{
			return (record.lifetime <= 0.0f);
		});

		if (element != m_debugRippleInfos.end())
		{
			element->info = info;
			element->lifetime = maxLifetime;
		}
		else
		{
			m_debugRippleInfos.emplace_back(info, maxLifetime);
		}
	}

	CRY_ASSERT(m_debugRippleInfos.size() <= maxRippleDebugCount);

	if (m_pCVarWaterRipplesDebug && m_pCVarWaterRipplesDebug->GetIVal() > 0)
	{
		auto* pAuxRenderer = gRenDev->GetIRenderAuxGeom();

		const SAuxGeomRenderFlags oldRenderFlags = pAuxRenderer->GetRenderFlags();

		pAuxRenderer->SetRenderFlags(SAuxGeomRenderFlags());

		for (auto& debugInfo : m_debugRippleInfos)
		{
			if (debugInfo.lifetime > 0.0f)
			{
				Vec3 vHitPos(debugInfo.info.position.x, debugInfo.info.position.y, debugInfo.info.position.z);
				pAuxRenderer->DrawSphere(vHitPos, 0.15f, ColorB(255, 0, 0, 255));
			}
		}

		Vec3 simCenter = Vec3(m_simOrigin.x, m_simOrigin.y, gRenDev->GetRCamera().vOrigin.z - 1.5f);
		pAuxRenderer->DrawSphere(simCenter, 0.15f, ColorB(0, 255, 0, 255));

		pAuxRenderer->SetRenderFlags(oldRenderFlags);
	}
#endif
}
