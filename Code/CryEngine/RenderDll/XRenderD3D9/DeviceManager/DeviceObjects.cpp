// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DeviceObjects.h"
#include "DriverD3D.h"
#include "xxhash.h"
#include "../../Common/ReverseDepth.h"
#include "../../Common/Textures/TextureHelpers.h"
#include "../GraphicsPipeline/Common/GraphicsPipelineStateSet.h"

extern uint8 g_StencilFuncLookup[8];
extern uint8 g_StencilOpLookup[8];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::array<SDeviceObjectHelpers::SShaderInstanceInfo, eHWSC_Num> SDeviceObjectHelpers::GetShaderInstanceInfo(::CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags, UPipelineState pipelineState[eHWSC_Num], bool bAllowTesselation)
{
	std::array<SDeviceObjectHelpers::SShaderInstanceInfo, eHWSC_Num> result;
	if (SShaderTechnique* pShaderTechnique = pShader->mfFindTechnique(technique))
	{
		if (pShaderTechnique->m_Passes.empty())
			return result;

		SShaderPass& shaderPass = pShaderTechnique->m_Passes[0];

		CHWShader* pHWShaders[] =
		{
			shaderPass.m_VShader,
			shaderPass.m_PShader,
			shaderPass.m_GShader,
			shaderPass.m_CShader,
			shaderPass.m_DShader,
			shaderPass.m_HShader,
		};

		for (EHWShaderClass shaderStage = eHWSC_Vertex; shaderStage < eHWSC_Num; shaderStage = EHWShaderClass(shaderStage + 1))
		{
			if (!bAllowTesselation && (shaderStage == eHWSC_Hull || shaderStage == eHWSC_Domain))
			{
				continue;
			}

			CHWShader_D3D* pHWShaderD3D = reinterpret_cast<CHWShader_D3D*>(pHWShaders[shaderStage]);
			result[shaderStage].pHwShader = pHWShaderD3D;
			result[shaderStage].technique = technique;

			if (pHWShaderD3D)
			{
				SShaderCombIdent Ident;
				Ident.m_LightMask = 0;
				Ident.m_RTMask = rtFlags & pHWShaderD3D->m_nMaskAnd_RT | pHWShaderD3D->m_nMaskOr_RT;
				Ident.m_MDMask = mdFlags & (shaderStage != eHWSC_Pixel ? 0xFFFFFFFF : ~HWMD_TEXCOORD_FLAG_MASK);
				Ident.m_MDVMask = ((shaderStage != eHWSC_Pixel) ? mdvFlags : 0) | CParserBin::m_nPlatform;
				Ident.m_GLMask = pHWShaderD3D->m_nMaskGenShader;
				Ident.m_pipelineState = pipelineState ? pipelineState[shaderStage] : UPipelineState();

				if (auto pInstance = pHWShaderD3D->mfGetInstance(pShader, Ident, 0))
				{
					if (pHWShaderD3D->mfCheckActivation(pShader, pInstance, 0))
					{
						result[shaderStage].pHwShaderInstance = pInstance;
						result[shaderStage].pDeviceShader = pInstance->m_Handle.m_pShader->m_pHandle;
					}
				}
			}
		}
	}

	return result;
}

bool SDeviceObjectHelpers::CheckTessellationSupport(SShaderItem& shaderItem)
{
	// TTYPE_ZPREPASS doesn't support tessellation
	EShaderTechniqueID techniquesInUse[] = { TTYPE_Z, TTYPE_SHADOWGEN };
	::CShader* pShader = reinterpret_cast<::CShader*>(shaderItem.m_pShader);

	bool bResult = false;
	for (int i = 0; i < CRY_ARRAY_COUNT(techniquesInUse); ++i)
	{
		if (auto pShaderTechnique = pShader->GetTechnique(shaderItem.m_nTechnique, techniquesInUse[i], true))
		{
			SShaderPass& shaderPass = pShaderTechnique->m_Passes[0];
			bResult |= (shaderPass.m_DShader && shaderPass.m_HShader);
		}
	}

	return bResult;
}

SDeviceObjectHelpers::CShaderConstantManager::CShaderConstantManager()
{
}

SDeviceObjectHelpers::CShaderConstantManager::CShaderConstantManager(CShaderConstantManager&& other)
{
	std::swap(m_constantBuffers, other.m_constantBuffers);
	std::swap(m_pShaderReflection, other.m_pShaderReflection);

	other.m_constantBuffers.clear();
	other.m_pShaderReflection.reset();
}

void SDeviceObjectHelpers::CShaderConstantManager::Reset()
{
	m_constantBuffers.clear();
}

bool SDeviceObjectHelpers::CShaderConstantManager::InitShaderReflection(::CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags)
{
	if (!m_pShaderReflection)
		m_pShaderReflection = CryMakeUnique<SShaderReflection>();

	// Release buffers that were added from previous reflection
	ReleaseReflectedBuffers();

	m_pShaderReflection->bValid = false;
	m_pShaderReflection->pShader = nullptr;
	m_pShaderReflection->technique.reset();
	m_pShaderReflection->bufferUpdateContexts.clear();

	auto shaderInfo = GetShaderInstanceInfo(pShader, technique, rtFlags, 0, 0, nullptr, false);
	std::vector<SConstantBufferBindInfo> bufferBindInfo;
	std::vector<SReflectedBufferUpdateContext> bufferUpdateContext;

	for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass < eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
	{
		if (shaderInfo[shaderClass].pHwShader)
		{
			if (!shaderInfo[shaderClass].pHwShaderInstance) // shader instance is not compiled yet
				return false;

			VectorSet<EConstantBufferShaderSlot> usedBuffersSlots;
			auto* pInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(shaderInfo[shaderClass].pHwShaderInstance);
			auto* pHwShader = reinterpret_cast<CHWShader_D3D*>(shaderInfo[shaderClass].pHwShader);
			const int vectorCount[] =
			{
				pInstance->m_nMaxVecs[eConstantBufferShaderSlot_PerBatch],
				pInstance->m_nMaxVecs[eConstantBufferShaderSlot_PerInstanceLegacy],
				CHWShader_D3D::s_nMax_PF_Vecs[shaderClass]
			};

			int maxVectorCount = 0;
			for (const auto& bind : pInstance->m_pBindVars)
			{
				if ((bind.m_dwBind & (SHADER_BIND_TEXTURE | SHADER_BIND_SAMPLER)) == 0
				    && bind.m_dwCBufSlot <= eConstantBufferShaderSlot_PerInstanceLegacy)
				{
					usedBuffersSlots.insert(EConstantBufferShaderSlot(bind.m_dwCBufSlot));
					maxVectorCount = std::max(maxVectorCount, vectorCount[bind.m_dwCBufSlot]);
				}
			}

			// NOTE: Get aligned stack-space (pointer and size aligned to manager's alignment requirement)
			CryStackAllocWithSizeVectorCleared(Vec4, maxVectorCount, zeroMem, CDeviceBufferManager::AlignBufferSizeForStreaming);

			for (auto bufferSlot : usedBuffersSlots)
			{
				CRY_ASSERT(bufferSlot >= eConstantBufferShaderSlot_PerBatch && bufferSlot <= eConstantBufferShaderSlot_PerInstanceLegacy);
				const size_t bufferSize = sizeof(Vec4) * vectorCount[bufferSlot];
				const size_t updateSize = CDeviceBufferManager::AlignBufferSizeForStreaming(bufferSize);

				SReflectedBufferUpdateContext updateContext;
				updateContext.vectorCount = vectorCount[bufferSlot];
				updateContext.pPreviousBuffer = nullptr;
				updateContext.shaderClass = shaderClass;
				updateContext.shaderInfo.pHwShader = shaderInfo[shaderClass].pHwShader;
				updateContext.shaderInfo.pHwShaderInstance = shaderInfo[shaderClass].pHwShaderInstance;
				updateContext.shaderInfo.pDeviceShader = shaderInfo[shaderClass].pDeviceShader;
				bufferUpdateContext.push_back(updateContext);

				SConstantBufferBindInfo bindInfo;
				bindInfo.shaderSlot = bufferSlot;
				bindInfo.shaderStages = EShaderStage(BIT(shaderClass));
				bindInfo.pBuffer = gcpRendD3D->m_DevBufMan.CreateConstantBuffer(bufferSize);
				bindInfo.pBuffer->UpdateBuffer(zeroMem, updateSize);
				bufferBindInfo.push_back(std::move(bindInfo));
			}
		}
	}

	// Update buffers here to prevent partial results from being visible, even though we return false if anything goes wrong.
	for (int i = 0, end = bufferUpdateContext.size(); i < end; ++i)
	{
		const int bufferIndex = m_constantBuffers.size();
		auto it = m_pShaderReflection->bufferUpdateContexts.insert(std::make_pair(bufferIndex, SReflectedBufferUpdateContext())).first;
		m_constantBuffers.push_back(SConstantBufferBindInfo());

		std::swap(m_constantBuffers.back(), bufferBindInfo[i]);
		std::swap(it->second, bufferUpdateContext[i]);
	}

	m_pShaderReflection->bValid = true;
	m_pShaderReflection->pShader = pShader;
	m_pShaderReflection->technique = technique;

	return true;
}

void SDeviceObjectHelpers::CShaderConstantManager::ReleaseShaderReflection()
{
	ReleaseReflectedBuffers();
	m_pShaderReflection.reset();
}

void SDeviceObjectHelpers::CShaderConstantManager::ReleaseReflectedBuffers()
{
	if (m_pShaderReflection)
	{
		// remove any buffers that were used for shader reflection
		if (!m_pShaderReflection->bufferUpdateContexts.empty())
		{
			int bufferCount = 0;
			for (int i = 0, end = m_constantBuffers.size(); i < end; ++i)
			{
				if (m_pShaderReflection->bufferUpdateContexts.find(i) == m_pShaderReflection->bufferUpdateContexts.end())
				{
					if (i != bufferCount)
					{
						std::swap(m_constantBuffers[bufferCount], m_constantBuffers[i]);
					}
					++bufferCount;
				}
			}

			m_constantBuffers.resize(bufferCount);
		}
	}
}

void SDeviceObjectHelpers::CShaderConstantManager::BeginNamedConstantUpdate()
{
	CRY_ASSERT(m_pShaderReflection);
	if (!m_pShaderReflection->bValid)
		return;

	uint32 numPasses;
	m_pShaderReflection->pShader->FXSetTechnique(m_pShaderReflection->technique);
	m_pShaderReflection->pShader->FXBegin(&numPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	// replace engine constant buffers with our own
	for (auto& it : m_pShaderReflection->bufferUpdateContexts)
	{
		const int bufferIndex = it.first;
		CRY_ASSERT(bufferIndex >= 0 && bufferIndex < m_constantBuffers.size());

		SReflectedBufferUpdateContext& uc = it.second;
		const SConstantBufferBindInfo& cb = m_constantBuffers[bufferIndex];

		if (CHWShader_D3D::s_pDataCB[uc.shaderClass][cb.shaderSlot])
			CHWShader_D3D::mfCommitCB(cb.shaderSlot, uc.shaderClass, uc.shaderClass);

		uc.pPreviousBuffer = std::move(CHWShader_D3D::s_pCB[uc.shaderClass][cb.shaderSlot][uc.vectorCount]);
		CHWShader_D3D::s_pCB[uc.shaderClass][cb.shaderSlot][uc.vectorCount] = cb.pBuffer;
		CHWShader_D3D::s_nCurMaxVecs[uc.shaderClass][cb.shaderSlot] = uc.vectorCount;

		uc.shaderInfo.pHwShader->m_pCurInst = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(uc.shaderInfo.pHwShaderInstance);
		CRY_ASSERT(uc.shaderInfo.pHwShader->m_pCurInst->m_nMaxVecs[cb.shaderSlot] == uc.vectorCount);
	}
}

void SDeviceObjectHelpers::CShaderConstantManager::EndNamedConstantUpdate()
{
	CRY_ASSERT(m_pShaderReflection);
	if (!m_pShaderReflection->bValid)
		return;

	if (!m_pShaderReflection->bufferUpdateContexts.empty())
	{
		// set per batch and per instance parameters
		auto setParams = [&](EHWShaderClass shaderClass, bool bPerBatch, bool bPerInstance)
		{
			for (auto& it : m_pShaderReflection->bufferUpdateContexts)
			{
				const SReflectedBufferUpdateContext& uc = it.second;
				if (uc.shaderClass == shaderClass)
				{
					auto pHwShader = uc.shaderInfo.pHwShader;
					auto pHwShaderInstance = reinterpret_cast<CHWShader_D3D::SHWSInstance*>(uc.shaderInfo.pHwShaderInstance);

					if (pHwShader && pHwShaderInstance)
					{
						pHwShader->m_pCurInst = pHwShaderInstance;

						if (bPerBatch)    pHwShader->mfSetParametersPB();
						if (bPerInstance) pHwShader->mfSetParametersPI(NULL, NULL);
					}

					break;
				}
			}
		};

		setParams(eHWSC_Pixel, true, true);
		setParams(eHWSC_Vertex, true, true);
		setParams(eHWSC_Geometry, true, false);
		setParams(eHWSC_Compute, true, false);
		setParams(eHWSC_Compute, false, false);

		// finalize constant buffers
		for (auto& it : m_pShaderReflection->bufferUpdateContexts)
		{
			const int bufferIndex = it.first;
			CRY_ASSERT(bufferIndex >= 0 && bufferIndex < m_constantBuffers.size());

			const SReflectedBufferUpdateContext& uc = it.second;
			const SConstantBufferBindInfo& cb = m_constantBuffers[bufferIndex];

			if (cb.pBuffer->m_buffer && CHWShader_D3D::s_pDataCB[uc.shaderClass][cb.shaderSlot])
			{
				CRY_ASSERT(CHWShader_D3D::s_pCB[uc.shaderClass][cb.shaderSlot][uc.vectorCount] == cb.pBuffer);
				cb.pBuffer->EndWrite();
			}

			CHWShader_D3D::s_pDataCB[uc.shaderClass][cb.shaderSlot] = NULL;
			CHWShader_D3D::s_pCurDevCB[uc.shaderClass][cb.shaderSlot] = cb.pBuffer->GetCode();

			// restore previous buffer
			CHWShader_D3D::s_pCB[uc.shaderClass][cb.shaderSlot][uc.vectorCount] = uc.pPreviousBuffer;
		}
	}

	m_pShaderReflection->pShader->FXEndPass();
	m_pShaderReflection->pShader->FXEnd();
}

bool SDeviceObjectHelpers::CShaderConstantManager::SetTypedConstantBuffer(EConstantBufferShaderSlot shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages)
{
	for (int i = 0, end = m_constantBuffers.size(); i < end; ++i)
	{
		auto& cb = m_constantBuffers[i];
		if (cb.shaderSlot == shaderSlot && cb.shaderStages == shaderStages)
		{
			if (cb.pBuffer != pBuffer)
			{
				CRY_ASSERT(!m_pShaderReflection || m_pShaderReflection->bufferUpdateContexts.find(i) == m_pShaderReflection->bufferUpdateContexts.end());
				cb.pBuffer = pBuffer;

				return true;
			}
			return false;
		}
	}

	m_constantBuffers.push_back(SConstantBufferBindInfo());
	SConstantBufferBindInfo& cb = m_constantBuffers.back();
	cb.shaderSlot = shaderSlot;
	cb.shaderStages = shaderStages;
	cb.pBuffer = pBuffer;

	return true;
}

bool SDeviceObjectHelpers::CShaderConstantManager::SetNamedConstant(const CCryNameR& paramName, const Vec4 param, EHWShaderClass shaderClass)
{
	return SetNamedConstantArray(paramName, &param, 1, shaderClass);
}

bool SDeviceObjectHelpers::CShaderConstantManager::SetNamedConstantArray(const CCryNameR& paramName, const Vec4 params[], uint32 numParams, EHWShaderClass shaderClass)
{
	CRY_ASSERT_MESSAGE(m_pShaderReflection, "Flag eFlags_ReflectConstantBuffersFromShader might be required for pass");
	if (!m_pShaderReflection->bValid)
		return false;

	auto pShader = m_pShaderReflection->pShader;
	switch (shaderClass) // TODO: implement DS and HS version
	{
	case eHWSC_Vertex:
		return pShader->FXSetVSFloat(paramName, params, numParams);
	case eHWSC_Pixel:
		return pShader->FXSetPSFloat(paramName, params, numParams);
	case eHWSC_Geometry:
		return pShader->FXSetGSFloat(paramName, params, numParams);
	case eHWSC_Compute:
		return pShader->FXSetCSFloat(paramName, params, numParams);
	default:
		CRY_ASSERT(false);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceGraphicsPSODesc::CDeviceGraphicsPSODesc(CDeviceResourceLayout* pResourceLayout, const SGraphicsPipelineStateDescription& pipelineDesc)
{
	InitWithDefaults();

	m_pResourceLayout = pResourceLayout;
	m_pShader = reinterpret_cast<::CShader*>(pipelineDesc.shaderItem.m_pShader);
	if (auto pTech = m_pShader->GetTechnique(pipelineDesc.shaderItem.m_nTechnique, pipelineDesc.technique, true))
	{
		m_technique = pTech->m_NameCRC;
		m_RenderState = pTech->m_Passes[0].m_RenderState;
	}

	m_ShaderFlags_RT = pipelineDesc.objectRuntimeMask;
	m_ShaderFlags_MDV = pipelineDesc.objectFlags_MDV;
	m_VertexFormat = pipelineDesc.vertexFormat;
	m_ObjectStreamMask = pipelineDesc.streamMask;
	m_PrimitiveType = (ERenderPrimitiveType)pipelineDesc.primitiveType;
}

CDeviceGraphicsPSODesc::CDeviceGraphicsPSODesc(uint32 renderTargetCount, CTexture* const* pRenderTargets, const SDepthTexture* pDepthTarget, SResourceView::KeyType* pRenderTargetViews)
{
	InitWithDefaults();

	for (int i = 0; i < renderTargetCount; ++i)
	{
		ETEX_Format rtFormat = pRenderTargets[i] ? pRenderTargets[i]->GetDstFormat() : eTF_Unknown;
		if (pRenderTargetViews && pRenderTargetViews[i] != SResourceView::DefaultRendertargetView)
		{
			rtFormat = (ETEX_Format)SResourceView(pRenderTargetViews[i]).m_Desc.nFormat;
		}

		m_RenderTargetFormats[i] = rtFormat;
	}

	m_DepthStencilFormat = (pDepthTarget && pDepthTarget->pTexture) ? pDepthTarget->pTexture->GetDstFormat() : eTF_Unknown;
}

void CDeviceGraphicsPSODesc::InitWithDefaults()
{
	ZeroStruct(*this);

	m_StencilState = STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP);
	m_StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	m_StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	m_VertexFormat = eVF_P3F_C4B_T2S;
	m_CullMode = eCULL_Back;
	m_PrimitiveType = eptTriangleList;
	m_DepthStencilFormat = eTF_Unknown;
	m_RenderTargetFormats.fill(eTF_Unknown);
	m_bDepthClip = true;
}

CDeviceGraphicsPSODesc& CDeviceGraphicsPSODesc::operator=(const CDeviceGraphicsPSODesc& other)
{
	// NOTE: need to make sure both structs are binary equivalent for hashing and comparison
	memcpy(this, &other, sizeof(CDeviceGraphicsPSODesc));
	if (m_pShader) m_pShader->AddRef();

	return *this;
}

CDeviceGraphicsPSODesc::CDeviceGraphicsPSODesc(const CDeviceGraphicsPSODesc& other)
{
	*this = other;
}

bool CDeviceGraphicsPSODesc::operator==(const CDeviceGraphicsPSODesc& other) const
{
	return memcmp(this, &other, sizeof(CDeviceGraphicsPSODesc)) == 0;
}

uint64 CDeviceGraphicsPSODesc::GetHash() const
{
	uint64 key = XXH64(this, sizeof(CDeviceGraphicsPSODesc), 0);
	return key;
}

void CDeviceGraphicsPSODesc::FillDescs(D3D11_RASTERIZER_DESC& rasterizerDesc, D3D11_BLEND_DESC& blendDesc, D3D11_DEPTH_STENCIL_DESC& depthStencilDesc) const
{
	// 00001110 11111111 11111111 11111111 USED
	// 00001110 11100000 00000000 11111111 BLENDMASK
	// ======== ======== ======== ======== ==================================
	// 00000000 00000000 00000000 00001111 BLSRC
	// 00000000 00000000 00000000 11110000 BLDST
	// 00000000 00000000 00000001 00000000 DEPTHWRITE
	// 00000000 00000000 00000010 00000000 DEPTHTEST
	// 00000000 00000000 00000100 00000000 STENCIL
	// 00000000 00000000 00001000 00000000 ALPHATEST
	// 00000000 00000000 11110000 00000000 NOCOLMASK
	// 00000000 00000011 00000000 00000000 POINT|WIREFRAME
	// 00000000 00011100 00000000 00000000 DEPTHFUNC
	// 00000000 11100000 00000000 00000000 BLENDOP
	// 00001110 00000000 00000000 00000000 BLENDOPALPHA
	const uint32 renderState = m_RenderState;

	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	// Fill mode
	rasterizerDesc.DepthClipEnable = m_bDepthClip ? 1 : 0;
	rasterizerDesc.FrontCounterClockwise = 1;
	rasterizerDesc.FillMode = (renderState & GS_WIREFRAME) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	rasterizerDesc.CullMode =
	  (m_CullMode == eCULL_Back)
	  ? D3D11_CULL_BACK
	  : ((m_CullMode == eCULL_None) ? D3D11_CULL_NONE : D3D11_CULL_FRONT);

	// *INDENT-OFF*
	// Blend state
	{
		const bool bBlendEnable = (renderState & GS_BLEND_MASK) != 0;

		for (size_t i = 0; i < CD3D9Renderer::RT_STACK_WIDTH; ++i)
			blendDesc.RenderTarget[i].BlendEnable = bBlendEnable;

		if (bBlendEnable)
		{
			struct BlendFactors
			{
				D3D11_BLEND BlendColor;
				D3D11_BLEND BlendAlpha;
			};

			static BlendFactors SrcBlendFactors[GS_BLSRC_MASK >> GS_BLSRC_SHIFT] =
			{
				{ (D3D11_BLEND)0,             (D3D11_BLEND)0             },        // UNINITIALIZED BLEND FACTOR
				{ D3D11_BLEND_ZERO,           D3D11_BLEND_ZERO           },        // GS_BLSRC_ZERO
				{ D3D11_BLEND_ONE,            D3D11_BLEND_ONE            },        // GS_BLSRC_ONE
				{ D3D11_BLEND_DEST_COLOR,     D3D11_BLEND_DEST_ALPHA     },        // GS_BLSRC_DSTCOL
				{ D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_INV_DEST_ALPHA },        // GS_BLSRC_ONEMINUSDSTCOL
				{ D3D11_BLEND_SRC_ALPHA,      D3D11_BLEND_SRC_ALPHA      },        // GS_BLSRC_SRCALPHA
				{ D3D11_BLEND_INV_SRC_ALPHA,  D3D11_BLEND_INV_SRC_ALPHA  },        // GS_BLSRC_ONEMINUSSRCALPHA
				{ D3D11_BLEND_DEST_ALPHA,     D3D11_BLEND_DEST_ALPHA     },        // GS_BLSRC_DSTALPHA
				{ D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA },        // GS_BLSRC_ONEMINUSDSTALPHA
				{ D3D11_BLEND_SRC_ALPHA_SAT,  D3D11_BLEND_SRC_ALPHA_SAT  },        // GS_BLSRC_ALPHASATURATE
				{ D3D11_BLEND_SRC_ALPHA,      D3D11_BLEND_ZERO           },        // GS_BLSRC_SRCALPHA_A_ZERO
				{ D3D11_BLEND_SRC1_ALPHA,     D3D11_BLEND_SRC1_ALPHA     },        // GS_BLSRC_SRC1ALPHA
			};

			static BlendFactors DstBlendFactors[GS_BLDST_MASK >> GS_BLDST_SHIFT] =
			{
				{ (D3D11_BLEND)0,             (D3D11_BLEND)0             },        // UNINITIALIZED BLEND FACTOR
				{ D3D11_BLEND_ZERO,           D3D11_BLEND_ZERO           },        // GS_BLDST_ZERO
				{ D3D11_BLEND_ONE,            D3D11_BLEND_ONE            },        // GS_BLDST_ONE
				{ D3D11_BLEND_SRC_COLOR,      D3D11_BLEND_SRC_ALPHA      },        // GS_BLDST_SRCCOL
				{ D3D11_BLEND_INV_SRC_COLOR,  D3D11_BLEND_INV_SRC_ALPHA  },        // GS_BLDST_ONEMINUSSRCCOL
				{ D3D11_BLEND_SRC_ALPHA,      D3D11_BLEND_SRC_ALPHA      },        // GS_BLDST_SRCALPHA
				{ D3D11_BLEND_INV_SRC_ALPHA,  D3D11_BLEND_INV_SRC_ALPHA  },        // GS_BLDST_ONEMINUSSRCALPHA
				{ D3D11_BLEND_DEST_ALPHA,     D3D11_BLEND_DEST_ALPHA     },        // GS_BLDST_DSTALPHA
				{ D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA },        // GS_BLDST_ONEMINUSDSTALPHA
				{ D3D11_BLEND_ONE,            D3D11_BLEND_ZERO           },        // GS_BLDST_ONE_A_ZERO
				{ D3D11_BLEND_INV_SRC1_ALPHA, D3D11_BLEND_INV_SRC1_ALPHA },        // GS_BLDST_ONEMINUSSRC1ALPHA
			};

			static D3D11_BLEND_OP BlendOp[GS_BLEND_OP_MASK >> GS_BLEND_OP_SHIFT] =
			{
				D3D11_BLEND_OP_ADD,          // 0 (unspecified): Default
				D3D11_BLEND_OP_MAX,          // GS_BLOP_MAX / GS_BLALPHA_MAX
				D3D11_BLEND_OP_MIN,          // GS_BLOP_MIN / GS_BLALPHA_MIN
				D3D11_BLEND_OP_SUBTRACT,     // GS_BLOP_SUB / GS_BLALPHA_SUB
				D3D11_BLEND_OP_REV_SUBTRACT, // GS_BLOP_SUBREV / GS_BLALPHA_SUBREV
			};

			blendDesc.RenderTarget[0].SrcBlend       = SrcBlendFactors[(renderState & GS_BLSRC_MASK) >> GS_BLSRC_SHIFT].BlendColor;
			blendDesc.RenderTarget[0].SrcBlendAlpha  = SrcBlendFactors[(renderState & GS_BLSRC_MASK) >> GS_BLSRC_SHIFT].BlendAlpha;
			blendDesc.RenderTarget[0].DestBlend      = DstBlendFactors[(renderState & GS_BLDST_MASK) >> GS_BLDST_SHIFT].BlendColor;
			blendDesc.RenderTarget[0].DestBlendAlpha = DstBlendFactors[(renderState & GS_BLDST_MASK) >> GS_BLDST_SHIFT].BlendAlpha;

			for (size_t i = 0; i < CD3D9Renderer::RT_STACK_WIDTH; ++i)
			{
				blendDesc.RenderTarget[i].BlendOp      = BlendOp[(renderState & GS_BLEND_OP_MASK) >> GS_BLEND_OP_SHIFT];
				blendDesc.RenderTarget[i].BlendOpAlpha = BlendOp[(renderState & GS_BLALPHA_MASK ) >> GS_BLALPHA_SHIFT ];
			}

			if ((renderState & GS_BLALPHA_MASK) == GS_BLALPHA_MIN)
			{
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
			}
		}
	}

	// Color mask
	{
		uint32 mask = 0xfffffff0 | ((renderState & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
		mask = (~mask) & 0xf;
		for (uint32 i = 0; i < CD3D9Renderer::RT_STACK_WIDTH; ++i)
			blendDesc.RenderTarget[i].RenderTargetWriteMask = mask;
	}

	// Depth-Stencil
	{
		depthStencilDesc.DepthWriteMask =  (renderState & GS_DEPTHWRITE) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthEnable    = ((renderState & GS_NODEPTHTEST) && !(renderState & GS_DEPTHWRITE)) ? 0 : 1;

		static D3D11_COMPARISON_FUNC DepthFunc[GS_DEPTHFUNC_MASK >> GS_DEPTHFUNC_SHIFT] =
		{
			D3D11_COMPARISON_LESS_EQUAL,     // GS_DEPTHFUNC_LEQUAL
			D3D11_COMPARISON_EQUAL,          // GS_DEPTHFUNC_EQUAL
			D3D11_COMPARISON_GREATER,        // GS_DEPTHFUNC_GREAT
			D3D11_COMPARISON_LESS,           // GS_DEPTHFUNC_LESS
			D3D11_COMPARISON_GREATER_EQUAL,  // GS_DEPTHFUNC_GEQUAL
			D3D11_COMPARISON_NOT_EQUAL,      // GS_DEPTHFUNC_NOTEQUAL
		};

		depthStencilDesc.DepthFunc =
			(renderState & (GS_NODEPTHTEST|GS_DEPTHWRITE)) == (GS_NODEPTHTEST|GS_DEPTHWRITE)
			? D3D11_COMPARISON_ALWAYS
			: DepthFunc[(m_RenderState & GS_DEPTHFUNC_MASK) >> GS_DEPTHFUNC_SHIFT];

		depthStencilDesc.StencilEnable    = (renderState & GS_STENCIL) ? 1 : 0;
		depthStencilDesc.StencilReadMask  = m_StencilReadMask;
		depthStencilDesc.StencilWriteMask = m_StencilWriteMask;

		depthStencilDesc.FrontFace.StencilFunc        = (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[m_StencilState & FSS_STENCFUNC_MASK];
		depthStencilDesc.FrontFace.StencilFailOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & FSS_STENCFAIL_MASK) >> FSS_STENCFAIL_SHIFT];
		depthStencilDesc.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & FSS_STENCZFAIL_MASK) >> FSS_STENCZFAIL_SHIFT];
		depthStencilDesc.FrontFace.StencilPassOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & FSS_STENCPASS_MASK) >> FSS_STENCPASS_SHIFT];
		depthStencilDesc.BackFace                     = depthStencilDesc.FrontFace;

		if (m_StencilState & FSS_STENCIL_TWOSIDED)
		{
			depthStencilDesc.BackFace.StencilFunc        = (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[(m_StencilState & (FSS_STENCFUNC_MASK << FSS_CCW_SHIFT)) >> FSS_CCW_SHIFT];
			depthStencilDesc.BackFace.StencilFailOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & (FSS_STENCFAIL_MASK << FSS_CCW_SHIFT)) >> (FSS_STENCFAIL_SHIFT + FSS_CCW_SHIFT)];
			depthStencilDesc.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & (FSS_STENCZFAIL_MASK << FSS_CCW_SHIFT)) >> (FSS_STENCZFAIL_SHIFT + FSS_CCW_SHIFT)];
			depthStencilDesc.BackFace.StencilPassOp      = (D3D11_STENCIL_OP)g_StencilOpLookup[(m_StencilState & (FSS_STENCPASS_MASK << FSS_CCW_SHIFT)) >> (FSS_STENCPASS_SHIFT + FSS_CCW_SHIFT)];
		}
	}
	// *INDENT-ON*
}

uint8 CDeviceGraphicsPSODesc::CombineVertexStreamMasks(uint8 fromShader, uint8 fromObject) const
{
	uint8 result = fromShader;

	if (fromObject & VSM_NORMALS)
		result |= VSM_NORMALS;

	if (fromObject & BIT(VSF_QTANGENTS))
	{
		result &= ~VSM_TANGENTS;
		result |= BIT(VSF_QTANGENTS);
	}

	if (fromObject & VSM_INSTANCED)
		result |= VSM_INSTANCED;

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDeviceComputePSODesc::CDeviceComputePSODesc(CDeviceResourceLayout* pResourceLayout, const SComputePipelineStateDescription& pipelineDesc)
{
	InitWithDefaults();

	m_pResourceLayout = pResourceLayout;
	m_pShader = reinterpret_cast<::CShader*>(pipelineDesc.shaderItem.m_pShader);
	if (auto pTech = m_pShader->GetTechnique(pipelineDesc.shaderItem.m_nTechnique, pipelineDesc.technique, true))
	{
		m_technique = pTech->m_NameCRC;
	}

	m_ShaderFlags_RT = pipelineDesc.objectRuntimeMask;
	m_ShaderFlags_MDV = pipelineDesc.objectFlags_MDV;
}

CDeviceComputePSODesc::CDeviceComputePSODesc(CDeviceResourceLayout* pResourceLayout, ::CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags)
{
	InitWithDefaults();

	m_pResourceLayout = pResourceLayout;
	m_pShader = pShader;
	m_technique = technique;
	m_ShaderFlags_RT = rtFlags;
	m_ShaderFlags_MD = mdFlags;
	m_ShaderFlags_MDV = mdvFlags;
}

void CDeviceComputePSODesc::InitWithDefaults()
{
	ZeroStruct(*this);
}

CDeviceComputePSODesc& CDeviceComputePSODesc::operator=(const CDeviceComputePSODesc& other)
{
	// NOTE: need to make sure both structs are binary equivalent for hashing and comparison
	memcpy(this, &other, sizeof(CDeviceComputePSODesc));
	if (m_pShader) m_pShader->AddRef();

	return *this;
}

CDeviceComputePSODesc::CDeviceComputePSODesc(const CDeviceComputePSODesc& other)
{
	*this = other;
}

bool CDeviceComputePSODesc::operator==(const CDeviceComputePSODesc& other) const
{
	return memcmp(this, &other, sizeof(CDeviceComputePSODesc)) == 0;
}

uint64 CDeviceComputePSODesc::GetHash() const
{
	uint64 key = XXH64(this, sizeof(CDeviceComputePSODesc), 0);
	return key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static auto lambdaTextureCallback = [](void* set, uint32 dirtyFlags)
{
	reinterpret_cast<CDeviceResourceSet*>(set)->SetDirty(true);
};

CDeviceResourceSet::CDeviceResourceSet(EFlags flags)
	: m_bValid(false)
	, m_bDirty(true)
	, m_bEmpty(true)
	, m_bDirtyLayout(true)
{
	m_Flags = flags;
}

CDeviceResourceSet::CDeviceResourceSet(const CDeviceResourceSet& other)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	m_Textures        = other.m_Textures;
	m_Samplers        = other.m_Samplers;
	m_Buffers         = other.m_Buffers;
	m_ConstantBuffers = other.m_ConstantBuffers;

	for (auto& rsTexBind : m_Textures)
	{
		if (CTexture* pTex = rsTexBind.second.resource)
			pTex->AddInvalidateCallback(this, lambdaTextureCallback);
	}

	m_bValid       = false;
	m_bDirty       = true;
	m_bEmpty       = other.m_bEmpty;
	m_bDirtyLayout = other.m_bDirtyLayout;
	m_Flags        = other.m_Flags;
}

CDeviceResourceSet::~CDeviceResourceSet()
{
	Clear();
}

void CDeviceResourceSet::SetDirty(bool bDirty)
{
	m_bDirty = m_bDirty | bDirty;
}

void CDeviceResourceSet::Clear(bool bTextures, bool bConstantBuffers)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	bool bInvalidated = (m_Samplers.size() + m_Buffers.size()) != 0;

	if (bTextures)
	{
		bInvalidated |= (m_Textures.size()) != 0;

		for (auto& rsTexBind : m_Textures)
		{
			if (CTexture* pTex = rsTexBind.second.resource)
				pTex->RemoveInvalidateCallbacks(this);
		}

		m_Textures.clear();
	}

	if (bConstantBuffers)
	{
		bInvalidated |= (m_ConstantBuffers.size()) != 0;

		m_ConstantBuffers.clear();
	}

	m_Samplers.clear();
	m_Buffers.clear();

	m_bDirty       |= bInvalidated;
	m_bDirtyLayout |= bInvalidated;
	m_bEmpty        = m_Textures.empty() & m_ConstantBuffers.empty();
}

void CDeviceResourceSet::SetTexture(int shaderSlot, CTexture* pTexture, SResourceView::KeyType resourceViewID, ::EShaderStage shaderStages)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());
	CRY_ASSERT(pTexture != nullptr);
	CRY_ASSERT(SResourceView::IsShaderResourceView(resourceViewID) || SResourceView::IsUnorderedAccessView(resourceViewID));

	DeviceResourceBinding::SShaderSlot slot = { DeviceResourceBinding::eShaderSlotType_TextureAndBuffer, shaderSlot };
	if (SResourceView::IsUnorderedAccessView(resourceViewID))
		slot.type = DeviceResourceBinding::eShaderSlotType_UnorderedAccessView;

	auto insertResult = m_Textures.insert(std::make_pair(slot, STextureData(nullptr, resourceViewID, shaderStages))); // nullptr to avoid addref in case of early out
	STextureData& texData = insertResult.first->second;

	if (texData.resource.get() != pTexture || texData.shaderStages != shaderStages || texData.view != resourceViewID)
	{
		if (texData.resource != nullptr)
			texData.resource->RemoveInvalidateCallbacks(this);

		texData.resource = pTexture;
		texData.shaderStages = shaderStages;
		texData.view = resourceViewID;

		if (texData.resource != nullptr)
			texData.resource->AddInvalidateCallback(this, lambdaTextureCallback);

		m_bDirty = true;
		m_bEmpty = false;
		m_bDirtyLayout |= insertResult.second;
	}
}

void CDeviceResourceSet::SetSampler(int shaderSlot, int sampler, ::EShaderStage shaderStages)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	SSamplerData samplerDataNew(sampler, SResourceView::DefaultView, shaderStages);
	DeviceResourceBinding::SShaderSlot slot = { DeviceResourceBinding::eShaderSlotType_Sampler, shaderSlot };

	auto insertResult = m_Samplers.insert(std::make_pair(slot, samplerDataNew));
	SSamplerData& samplerData = insertResult.first->second;

	if (insertResult.second || samplerData.resource != sampler || samplerData.shaderStages != shaderStages)
	{
		samplerData = samplerDataNew;

		m_bDirty = true;
		m_bEmpty = false;
		m_bDirtyLayout |= insertResult.second;
	}
}

void CDeviceResourceSet::SetConstantBuffer(int shaderSlot, CConstantBuffer* pBuffer, ::EShaderStage shaderStages)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());
	CRY_ASSERT(pBuffer != nullptr);

	DeviceResourceBinding::SShaderSlot slot = { DeviceResourceBinding::eShaderSlotType_ConstantBuffer, shaderSlot };

	auto insertResult = m_ConstantBuffers.insert(std::make_pair(slot, SConstantBufferData(nullptr, SResourceView::DefaultView, shaderStages))); // nullptr to avoid addref in case of early out
	SConstantBufferData& cbData = insertResult.first->second;

	if (cbData.resource != pBuffer || cbData.shaderStages != shaderStages)
	{
		cbData.resource = pBuffer;
		cbData.shaderStages = shaderStages;

		m_bDirty = true;
		m_bEmpty = false;
		m_bDirtyLayout |= insertResult.second;
	}
}

void CDeviceResourceSet::SetBuffer(int shaderSlot, const CGpuBuffer& buffer, bool bUnorderedAccess, ::EShaderStage shaderStages)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	DeviceResourceBinding::SShaderSlot slot = { DeviceResourceBinding::eShaderSlotType_TextureAndBuffer, shaderSlot };
	if (bUnorderedAccess)
		slot.type = DeviceResourceBinding::eShaderSlotType_UnorderedAccessView;

	auto view = bUnorderedAccess ? SResourceView::DefaultUnordererdAccessView : SResourceView::DefaultView;
	auto insertResult = m_Buffers.insert(std::make_pair(slot, SBufferData(CGpuBuffer(), view, shaderStages))); // empty buffer to avoid addref in case of early out
	SBufferData& bufferData = insertResult.first->second;

	if (!(bufferData.resource == buffer) || bufferData.shaderStages != shaderStages)
	{
		bufferData.resource = buffer;
		bufferData.shaderStages = shaderStages;

		m_bDirty = true;
		m_bEmpty = false;
		m_bDirtyLayout = insertResult.second;
	}
}

::EShaderStage CDeviceResourceSet::GetShaderStages() const
{
	::EShaderStage result = EShaderStage_None;

	for (const auto& it : m_ConstantBuffers)
		result |= it.second.shaderStages;

	for (const auto& it : m_Textures)
		result |= it.second.shaderStages;

	for (const auto& it : m_Samplers)
		result |= it.second.shaderStages;

	return result;
}

bool CDeviceResourceSet::Fill(::CShader* pShader, CShaderResources* pResources, ::EShaderStage shaderStages)
{
	CRY_ASSERT(gRenDev->m_pRT->IsRenderThread());

	Clear(false, false);

	uint64 bindSlotMask = 0;
	static_assert(MAX_TMU <= sizeof(bindSlotMask) * 8, "Need a larger type for mask");

	for (auto& texture : m_Textures)
	{
		const uint32 bindSlot = texture.first.slotNumber;
		bindSlotMask |= 1ULL << bindSlot;
	}

	m_bEmpty = bindSlotMask == 0ULL;
	m_Flags  = EFlags(m_Flags & ~(EFlags_AnimatedSequence | EFlags_DynamicUpdates | EFlags_PendingAllocation));

	for (EEfResTextures texType = EFTT_DIFFUSE; texType < EFTT_MAX; texType = EEfResTextures(texType + 1))
	{
		SResourceView::KeyType view = SResourceView::DefaultView;
		CTexture* pTex = TextureHelpers::LookupTexDefault(texType);

		if (pResources->m_Textures[texType])
		{
			const STexSamplerRT& smp = pResources->m_Textures[texType]->m_Sampler;
			if (smp.m_pDynTexSource)
			{
				view = SResourceView::DefaultViewSRGB;
				m_Flags = EFlags(m_Flags | EFlags_DynamicUpdates);

				if (ITexture* pITex = smp.m_pDynTexSource->GetTexture())
				{
					m_Flags = EFlags(m_Flags & ~EFlags_PendingAllocation);
					pTex = CTexture::GetByID(pITex->GetTextureID());
				}
				else
				{
					m_Flags = EFlags(m_Flags | EFlags_PendingAllocation);
					pTex = CTexture::s_ptexBlackAlpha;
				}
			}
			else if (smp.m_pAnimInfo && (smp.m_pAnimInfo->m_NumAnimTexs > 1))
			{
				m_Flags = EFlags(m_Flags | EFlags_AnimatedSequence);
				pTex = smp.m_pTex;
			}
			else
			{
				pTex = smp.m_pTex;
			}
		}

		CRY_ASSERT(pTex);
		auto bindSlot = IShader::GetTextureSlot(texType);
		SetTexture(bindSlot, pTex, view, shaderStages);
		bindSlotMask &= ~(1ULL << bindSlot);
	}

	m_bDirty       |= bindSlotMask != 0ULL;
	m_bDirtyLayout |= bindSlotMask != 0ULL;

	// Any texture that was not set, reset it now.
	for (uint32 bindSlot = 0; bindSlotMask && (bindSlot < MAX_TMU); ++bindSlot)
	{
		if ((bindSlotMask & (1ULL << bindSlot)) != 0)
		{
			DeviceResourceBinding::SShaderSlot slot = { DeviceResourceBinding::eShaderSlotType_TextureAndBuffer, bindSlot };
			m_Textures.erase(slot);

			bindSlotMask &= ~(1ULL << bindSlot);
		}
	}

	SetConstantBuffer(eConstantBufferShaderSlot_PerMaterial, pResources->m_pCB, shaderStages);

	return true;
}

void CDeviceResourceSet::Build()
{
	m_bValid = false;
	EFlags flags = m_Flags;

	// check for dynamic data which requires runtime patching of the resource set
	{
		flags &= ~EFlags_Multibuffered;

		if (m_Flags & EFlags_ContainsVolatileBuffers || !m_ConstantBuffers.empty())
		{
			flags |= EFlags_Multibuffered;
		}
		else
		{
			for (auto buffer : m_Buffers)
			{
				if (buffer.second.resource.m_flags & DX11BUF_DYNAMIC)
				{
					flags |= EFlags_Multibuffered;
					break;
				}
			}
		}
	}

	if (BuildImpl(flags))
	{
		m_bValid = true;
		m_bDirty = false;
		m_bDirtyLayout = true;
		m_Flags = flags;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SDeviceResourceLayoutDesc::SetInlineConstants(uint32 numConstants)
{
	m_InlineConstantCount += numConstants;
}

void SDeviceResourceLayoutDesc::SetResourceSet(uint32 bindSlot, CDeviceResourceSetPtr pResourceSet)
{
	m_ResourceSets[bindSlot] = pResourceSet;
}

void SDeviceResourceLayoutDesc::SetConstantBuffer(uint32 bindSlot, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages)
{
	m_ConstantBuffers[bindSlot].shaderSlot = shaderSlot;
	m_ConstantBuffers[bindSlot].shaderStages = shaderStages;
}

// local helper functions for comparing arrays of resources by bind info
enum eComparisonResult { eLess = -1, eEqual, eGreater };

eComparisonResult compareResourceSlots(DeviceResourceBinding::SShaderSlot shaderSlotA, DeviceResourceBinding::SShaderSlot shaderSlotB)
{
	if (shaderSlotA.type != shaderSlotB.type)
		return shaderSlotA.type < shaderSlotB.type ? eLess : eGreater;

	if (shaderSlotA.slotNumber != shaderSlotB.slotNumber)
		return shaderSlotA < shaderSlotB ? eLess : eGreater;

	return eEqual;
}

eComparisonResult compareResourceSlots(EConstantBufferShaderSlot shaderSlotA, EConstantBufferShaderSlot shaderSlotB)
{
	if (shaderSlotA != shaderSlotB)
		return shaderSlotA < shaderSlotB ? eLess : eGreater;

	return eEqual;
}

template<typename T>
eComparisonResult compareResourceBindings(const T& resourceA, const T& resourceB)
{
	if (resourceA.shaderStages != resourceB.shaderStages)
		return resourceA.shaderStages < resourceB.shaderStages ? eLess : eGreater;

	return eEqual;
}

template<>
eComparisonResult compareResourceBindings<CDeviceResourceSet::SSamplerData>(const CDeviceResourceSet::SSamplerData& resourceA, const CDeviceResourceSet::SSamplerData& resourceB)
{
	// NOTE: Samplers are always static and injected into the layout
	if (resourceA.resource != resourceB.resource)
		return resourceA.resource < resourceB.resource ? eLess : eGreater;

	if (resourceA.shaderStages != resourceB.shaderStages)
		return resourceA.shaderStages < resourceB.shaderStages ? eLess : eGreater;

	return eEqual;
}

template<typename T>
eComparisonResult compareResourceLists(const T& a, const T& b)
{
	if (a.size() != b.size())
		return a.size() < b.size() ? eLess : eGreater;

	for (auto itA = a.begin(), itB = b.begin(); itA != a.end(); ++itA, ++itB)
	{
		if (auto cmp = compareResourceSlots(itA->first, itB->first))
			return cmp;

		if (auto cmp = compareResourceBindings(itA->second, itB->second))
			return cmp;
	}

	return eEqual;
}

bool SDeviceResourceLayoutDesc::operator<(const SDeviceResourceLayoutDesc& other) const
{
	if (m_InlineConstantCount != other.m_InlineConstantCount)
		return m_InlineConstantCount < other.m_InlineConstantCount;

	if (m_ResourceSets.size() != other.m_ResourceSets.size())
		return m_ResourceSets.size() < other.m_ResourceSets.size();

	if (m_ConstantBuffers.size() != other.m_ConstantBuffers.size())
		return m_ConstantBuffers.size() < other.m_ConstantBuffers.size();

	// constant buffers
	for (auto cb0 = m_ConstantBuffers.begin(), cb1 = other.m_ConstantBuffers.begin(); cb0 != m_ConstantBuffers.end(); ++cb0, ++cb1)
	{
		if (cb0->first != cb1->first)
			return cb0->first < cb1->first;

		if (auto cmp = compareResourceSlots(cb0->second.shaderSlot, cb1->second.shaderSlot))
			return cmp == eLess;

		if (auto cmp = compareResourceBindings(cb0->second, cb1->second))
			return cmp == eLess;
	}

	// resource sets
	for (auto rs0 = m_ResourceSets.begin(), rs1 = other.m_ResourceSets.begin(); rs0 != m_ResourceSets.end(); ++rs0, ++rs1)
	{
		if (rs0->first != rs1->first)
			return rs0->first < rs1->first;

		if (auto cmp = compareResourceLists(rs0->second->m_ConstantBuffers, rs1->second->m_ConstantBuffers))
			return cmp == eLess;

		if (auto cmp = compareResourceLists(rs0->second->m_Textures, rs1->second->m_Textures))
			return cmp == eLess;

		if (auto cmp = compareResourceLists(rs0->second->m_Buffers, rs1->second->m_Buffers))
			return cmp == eLess;

		if (auto cmp = compareResourceLists(rs0->second->m_Samplers, rs1->second->m_Samplers))
			return cmp == eLess;
	}

	return false;
}

UsedBindSlotSet SDeviceResourceLayoutDesc::GetRequiredResourceBindings() const
{
	UsedBindSlotSet result = 0;

	for (const auto& cb : m_ConstantBuffers)
		result[cb.first] = true;

	for (const auto& rs : m_ResourceSets)
		result[rs.first] = true;

	return result;
}

bool SDeviceResourceLayoutDesc::IsValid() const
{
	// VALIDATION RULES:
	// * Resource layout cannot be empty
	// * Cannot bind multiple resources to same layout (CPU side) slot
	// * Cannot have gaps in layout (CPU side) slots
	// * Cannot bind multiple resources to same shader slot
	//   -> Due to dx12, even things like tex0 => (t0, EShaderStage_Vertex), tex1 => (t0, EShaderStage_Pixel) are invalid
	// * There is a restriction on the GpuBuffer count per resource set. check 'ResourceSetBufferCount'

	struct resourceDebugInfo
	{
		enum EType { eType_Texture = 0, eType_Buffer, eType_ConstantBuffer };

		resourceDebugInfo(const CTexture* pTex)       : pTexture(pTex),        type(eType_Texture)        {}
		resourceDebugInfo(const CGpuBuffer* pBuf)     : pGpuBuffer(pBuf),      type(eType_Buffer)         {}
		resourceDebugInfo(const CConstantBuffer* pCb) : pConstantBuffer(pCb),  type(eType_ConstantBuffer) {}

		union 
		{
			const CTexture* pTexture;
			const CGpuBuffer* pGpuBuffer;
			const CConstantBuffer* pConstantBuffer;
		};

		EType type;

		const char* ToString()
		{
			switch (type)
			{
			case eType_Texture:        return pTexture->GetName();
			case eType_Buffer:         return "GpuBuffer";
			case eType_ConstantBuffer: return "ConstantBuffer";
			}
			return "Unknown";
		}
	};


	// Need to have at least one resource set or constant buffer/inline constants
	if (m_ResourceSets.empty() && m_ConstantBuffers.empty() && m_InlineConstantCount == 0)
	{
		CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: no data");
		return false;
	}

	// Check for overlapping resource set and constant buffer bind slots
	std::set<uint32> usedLayoutSlots;
	std::map<int16, resourceDebugInfo> usedShaderBindSlots[DeviceResourceBinding::eShaderSlotType_Count][eHWSC_Num]; // used slot numbers per slot type and shader stage

	if (m_InlineConstantCount > 0)
		usedLayoutSlots.insert(0);

	auto validateShaderBindSlot = [&](DeviceResourceBinding::SShaderSlot shaderSlot, ::EShaderStage shaderStages, resourceDebugInfo debugInfo)
	{
		for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
		{
			if (shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
			{
				auto insertResult = usedShaderBindSlots[shaderSlot.type][shaderClass].insert(std::make_pair(shaderSlot.slotNumber, debugInfo));
				if (insertResult.second == false)
				{
					char slotPrefix[] = { 'b', 't', 'u', 's' };
					auto& existingResource = insertResult.first->second;
					auto& currentResource = debugInfo;

					CRY_ASSERT_TRACE(false, ("Invalid Resource Layout : Multiple resources bound to shader slot %c%d: A: %s - B: %s",
						slotPrefix[shaderSlot.type], shaderSlot.slotNumber,
						existingResource.ToString(), currentResource.ToString()));

					return false;
				}
			}
		}

		return true;
	};

	auto validateLayoutSlot = [&](uint32 layoutSlot)
	{
		if (usedLayoutSlots.insert(layoutSlot).second == false)
		{
			CRY_ASSERT_TRACE(false, ("Invalid Resource Layout: Multiple resources on layout (CPU side) slot %d", layoutSlot));
			return false;
		}

		return true;
	};

	for (const auto& rs : m_ResourceSets)
	{
		const CDeviceResourceSetPtr& resourceSet = rs.second;

		if (resourceSet == nullptr)
		{
			CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: NULL resource set");
			return false;
		}

		if (!validateLayoutSlot(rs.first))
			return false;

		for (const auto& rsTexBind : resourceSet->m_Textures)
		{
			const DeviceResourceBinding::SShaderSlot slot = rsTexBind.first;
			const CDeviceResourceSet::STextureData& textureData = rsTexBind.second;
			bool bSrv = rsTexBind.second.view == SResourceView::DefaultView || SResourceView(rsTexBind.second.view).m_Desc.eViewType == SResourceView::eShaderResourceView;

			if (!validateShaderBindSlot(slot, textureData.shaderStages, resourceDebugInfo(textureData.resource)))
				return false;
		}

		std::array<int, eHWSC_Num> bufferCountPerShaderStage;
		bufferCountPerShaderStage.fill(0);

		for (const auto& rsBufferBind : resourceSet->m_Buffers)
		{
			const DeviceResourceBinding::SShaderSlot slot = rsBufferBind.first;
			const CDeviceResourceSet::SBufferData& bufferData = rsBufferBind.second;
			bool bSrv = bufferData.view == SResourceView::DefaultView || SResourceView(bufferData.view).m_Desc.eViewType == SResourceView::eShaderResourceView;

			if (!validateShaderBindSlot(slot, bufferData.shaderStages, resourceDebugInfo(&bufferData.resource)))
				return false;

			for (EHWShaderClass shaderClass = eHWSC_Vertex; shaderClass != eHWSC_Num; shaderClass = EHWShaderClass(shaderClass + 1))
			{
				if (bufferData.shaderStages & SHADERSTAGE_FROM_SHADERCLASS(shaderClass))
				{
					if (++bufferCountPerShaderStage[shaderClass] > ResourceSetBufferCount)
					{
						CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: Too many buffers in resource set. Consider increasing ResourceSetBufferCount");
						return false;
					}
				}
			}
		}

		for (const auto& rsCbBind : resourceSet->m_ConstantBuffers)
		{
			const DeviceResourceBinding::SShaderSlot slot = rsCbBind.first;
			const CDeviceResourceSet::SConstantBufferData& constantBufferData = rsCbBind.second;

			if (!validateShaderBindSlot(slot, constantBufferData.shaderStages, resourceDebugInfo(constantBufferData.resource)))
				return false;
		}
	}

	for (const auto& cbBind : m_ConstantBuffers)
	{
		const SInlineConstantBuffer& inlineCB = cbBind.second;

		if (!validateLayoutSlot(cbBind.first))
			return false;

		const DeviceResourceBinding::SShaderSlot slot = { DeviceResourceBinding::eShaderSlotType_ConstantBuffer, inlineCB.shaderSlot };
		CConstantBuffer* pDummyCB = nullptr;
		if (!validateShaderBindSlot(slot, inlineCB.shaderStages, resourceDebugInfo(pDummyCB)))
			return false;
	}

	// Make sure there are no 'holes' in the used binding slots
	{
		int previousSlot = -1;
		for (auto slot : usedLayoutSlots)
		{
			if (slot != previousSlot + 1)
			{
				CRY_ASSERT_MESSAGE(false, "Invalid Resource Layout: gap in layout (CPU side) slots");
				return false;
			}

			previousSlot = slot;
		}

		if (previousSlot > EResourceLayoutSlot_Max)
			return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeviceCommandList::Close()
{
#if 0
	ERenderListID renderList = EFSLIST_INVALID; // TODO: set to correct renderlist

	#if defined(ENABLE_PROFILING_CODE)
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nNumPSOSwitches, &m_numPSOSwitches);
	CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nNumLayoutSwitches, &m_numLayoutSwitches);
	CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nNumResourceSetSwitches, &m_numResourceSetSwitches);
	CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nNumInlineSets, &m_numInlineSets);
	CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs, &m_numDIPs);
	CryInterlockedAdd(&rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[renderList], &m_numPolygons);
	gcpRendD3D->GetGraphicsPipeline().IncrementNumInvalidDrawcalls(m_numInvalidDIPs);
	#endif
#endif

	CloseImpl();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDeviceGraphicsPSOPtr CDeviceObjectFactory::CreateGraphicsPSO(const CDeviceGraphicsPSODesc& psoDesc)
{
	auto it = m_GraphicsPsoCache.find(psoDesc);
	if (it != m_GraphicsPsoCache.end())
		return it->second;

	auto pPso = CreateGraphicsPSOImpl(psoDesc);
	m_GraphicsPsoCache.emplace(psoDesc, pPso);

	if (!pPso->IsValid())
		m_InvalidGraphicsPsos.emplace(psoDesc, pPso);

	return pPso;
}

CDeviceComputePSOPtr CDeviceObjectFactory::CreateComputePSO(const CDeviceComputePSODesc& psoDesc)
{
	auto it = m_ComputePsoCache.find(psoDesc);
	if (it != m_ComputePsoCache.end())
		return it->second;

	auto pPso = CreateComputePSOImpl(psoDesc);
	m_ComputePsoCache.emplace(psoDesc, pPso);

	if (!pPso->IsValid())
		m_InvalidComputePsos.emplace(psoDesc, pPso);

	return pPso;
}

CDeviceResourceLayoutPtr CDeviceObjectFactory::CreateResourceLayout(const SDeviceResourceLayoutDesc& resourceLayoutDesc)
{
	CDeviceResourceLayoutPtr pResult = nullptr;

	auto it = m_ResourceLayoutCache.find(resourceLayoutDesc);
	if (it != m_ResourceLayoutCache.end())
		return it->second;

	if (resourceLayoutDesc.IsValid())
	{
		// clone resource sets for cache to prevent modifications from outside code
		SDeviceResourceLayoutDesc layoutDesc = resourceLayoutDesc;
		for(auto& rs : layoutDesc.m_ResourceSets)
			rs.second = CCryDeviceWrapper::GetObjectFactory().CloneResourceSet(rs.second);

		if (pResult = CreateResourceLayoutImpl(layoutDesc))
		{
			m_ResourceLayoutCache[layoutDesc] = pResult;
		}
	}

	return pResult;
}

const CDeviceInputStream* CDeviceObjectFactory::CreateVertexStreamSet(uint32 numStreams, const SStreamInfo* streams)
{
	TVertexStreams vertexStreams = {};
	bool vertexFilled = false;

	for (int i = 0; i < numStreams; ++i)
		vertexFilled |= !!(vertexStreams[i] = streams[i]);

	return (vertexFilled ? m_uniqueVertexStreams.insert(vertexStreams).first->data() : nullptr);
}

const CDeviceInputStream* CDeviceObjectFactory::CreateIndexStreamSet(const SStreamInfo* stream)
{
	TIndexStreams indexStream = {};
	bool indexFilled = false;

	indexFilled |= !!(indexStream[0] = stream[0]);

	return (indexFilled ? m_uniqueIndexStreams.insert(indexStream).first->data() : nullptr);
}

void CDeviceObjectFactory::ReleaseResources()
{
	ReleaseResourcesImpl();

	m_GraphicsPsoCache.clear();
	m_InvalidGraphicsPsos.clear();

	m_ComputePsoCache.clear();
	m_InvalidComputePsos.clear();

	m_ResourceLayoutCache.clear();
}

void CDeviceObjectFactory::ReloadPipelineStates()
{
	for (auto& it : m_GraphicsPsoCache)
	{
		if (!it.second->Init(it.first))
			m_InvalidGraphicsPsos.emplace(it.first, it.second);
	}

	for (auto& it : m_ComputePsoCache)
	{
		if (!it.second->Init(it.first))
			m_InvalidComputePsos.emplace(it.first, it.second);
	}
}

void CDeviceObjectFactory::UpdatePipelineStates()
{
	for (auto it = m_InvalidGraphicsPsos.begin(), itEnd = m_InvalidGraphicsPsos.end(); it != itEnd; )
	{
		auto itCurrentPSO = it++;
		auto pPso = itCurrentPSO->second.lock();

		if (!pPso || pPso->Init(itCurrentPSO->first))
			m_InvalidGraphicsPsos.erase(itCurrentPSO);
	}

	for (auto it = m_InvalidComputePsos.begin(), itEnd = m_InvalidComputePsos.end(); it != itEnd; )
	{
		auto itCurrentPSO = it++;
		auto pPso = itCurrentPSO->second.lock();

		if (!pPso || pPso->Init(itCurrentPSO->first))
			m_InvalidComputePsos.erase(itCurrentPSO);
	}
}
