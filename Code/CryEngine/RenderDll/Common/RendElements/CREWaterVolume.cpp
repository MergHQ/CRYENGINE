// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/CREWaterVolume.h>

#include "XRenderD3D9/DriverD3D.h"

#include "GraphicsPipeline/Water.h"
#include "Common/ReverseDepth.h"
#include "Common/PostProcess/PostProcessUtils.h"

//////////////////////////////////////////////////////////////////////////

namespace watervolume
{
const buffer_handle_t invalidBufferHandle = ~0u;

struct SPerInstanceConstantBuffer
{
	Matrix44 PerInstanceWorldMatrix;

	// ocean related parameters
	Vec4 OceanParams0;
	Vec4 OceanParams1;

	// water surface and reflection parameters
	Vec4 vBBoxMin;
	Vec4 vBBoxMax;

	// Fog parameters
	Vec4 cFogColorDensity;
	Vec4 cViewerColorToWaterPlane;

	// water fog volume parameters
	Vec4 cFogPlane;
	Vec4 cPerpDist;
	Vec4 cFogColorShallowWaterLevel;
	Vec4 cUnderWaterInScatterConst;
	Vec4 volFogShadowRange;

	// water caustics parameters
	Vec4 vCausticParams;
};

struct SCompiledWaterVolume : NoCopy
{
	SCompiledWaterVolume()
		: m_pPerInstanceCB(nullptr)
		, m_vertexStreamSet(nullptr)
		, m_indexStreamSet(nullptr)
		, m_nVerticesCount(0)
		, m_nStartIndex(0)
		, m_nNumIndices(0)
		, m_bValid(0)
		, m_bHasTessellation(0)
		, m_bCaustics(0)
		, m_reserved(0)
	{}

	~SCompiledWaterVolume()
	{
		ReleaseDeviceResources();
	}

	void ReleaseDeviceResources()
	{
		if (m_pMaterialResourceSet)
		{
			gRenDev->m_pRT->RC_ReleaseRS(m_pMaterialResourceSet);
		}

		if (m_pPerInstanceResourceSet)
		{
			gRenDev->m_pRT->RC_ReleaseRS(m_pPerInstanceResourceSet);
		}

		if (m_pPerInstanceCB)
		{
			gRenDev->m_pRT->RC_ReleaseCB(m_pPerInstanceCB);
			m_pPerInstanceCB = nullptr;
		}
	}

	CDeviceResourceSetPtr     m_pMaterialResourceSet;
	CDeviceResourceSetPtr     m_pPerInstanceResourceSet;
	CConstantBuffer*          m_pPerInstanceCB;

	const CDeviceInputStream* m_vertexStreamSet;
	const CDeviceInputStream* m_indexStreamSet;

	int32                     m_nVerticesCount;
	int32                     m_nStartIndex;
	int32                     m_nNumIndices;

	DevicePipelineStatesArray m_psoArray;

	uint32                    m_bValid           : 1; //!< True if compilation succeeded.
	uint32                    m_bHasTessellation : 1; //!< True if tessellation is enabled
	uint32                    m_bCaustics        : 1; //!< True if this object can be rendered in caustics gen pass.
	uint32                    m_reserved         : 29;
};
}

//////////////////////////////////////////////////////////////////////////

CREWaterVolume::CREWaterVolume()
	: CRendElementBase()
	, m_pParams(nullptr)
	, m_pOceanParams(nullptr)
	, m_drawWaterSurface(false)
	, m_drawFastPath(false)
	, m_vertexBuffer()
	, m_indexBuffer()
	, m_vertexBufferQuad()
	, m_pCompiledObject(new watervolume::SCompiledWaterVolume())
{
	mfSetType(eDATA_WaterVolume);
	mfUpdateFlags(FCEF_TRANSFORM);
}

CREWaterVolume::~CREWaterVolume()
{
	if (m_vertexBuffer.handle != watervolume::invalidBufferHandle)
	{
		gRenDev->m_DevBufMan.Destroy(m_vertexBuffer.handle);
	}
	if (m_indexBuffer.handle != watervolume::invalidBufferHandle)
	{
		gRenDev->m_DevBufMan.Destroy(m_indexBuffer.handle);
	}
	if (m_vertexBufferQuad.handle != watervolume::invalidBufferHandle)
	{
		gRenDev->m_DevBufMan.Destroy(m_vertexBufferQuad.handle);
	}
}

void CREWaterVolume::mfGetPlane(Plane& pl)
{
	pl = m_pParams->m_fogPlane;
	pl.d = -pl.d;
}

void CREWaterVolume::mfCenter(Vec3& vCenter, CRenderObject* pObj)
{
	vCenter = m_pParams->m_center;
	if (pObj)
		vCenter += pObj->GetTranslation();
}

void CREWaterVolume::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

bool CREWaterVolume::mfDraw(CShader* ef, SShaderPass* sfm)
{
	assert(m_pParams);
	if (!m_pParams)
		return false;

	CD3D9Renderer* rd(gcpRendD3D);

	IF (ef->m_eShaderType != eST_Water, 0)
	{
#if !defined(_RELEASE)
		// CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Incorrect shader set for water / water fog volume");
#endif
		return false;
	}

	bool bCaustics = CRenderer::CV_r_watercaustics &&
	                 CRenderer::CV_r_watercausticsdeferred &&
	                 CRenderer::CV_r_watervolumecaustics &&
	                 m_pParams->m_caustics &&
	                 (-m_pParams->m_fogPlane.d >= 1.0f); // unfortunately packing to RG8 limits us to heights over 1 meter, so we just disable if volume goes below.

	// Don't render caustics pass unless needed.
	if ((rd->m_RP.m_nBatchFilter & FB_WATER_CAUSTIC) && !bCaustics)
		return false;

	uint64 nFlagsShaderRTSave = gcpRendD3D->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE5]);
	const bool renderFogShadowWater = (CRenderer::CV_r_FogShadowsWater > 0) && (m_pParams->m_fogShadowing > 0.01f);

	Vec4 volFogShadowRange(0, 0, clamp_tpl(m_pParams->m_fogShadowing, 0.0f, 1.0f), 1.0f - clamp_tpl(m_pParams->m_fogShadowing, 0.0f, 1.0f));
#if defined(VOLUMETRIC_FOG_SHADOWS)
	const bool renderFogShadow = gcpRendD3D->m_bVolFogShadowsEnabled;
	{
		Vec3 volFogShadowRangeP;
		gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_RANGE, volFogShadowRangeP);
		volFogShadowRangeP.x = clamp_tpl(volFogShadowRangeP.x, 0.01f, 1.0f);
		volFogShadowRange.x = volFogShadowRangeP.x;
		volFogShadowRange.y = volFogShadowRangeP.y;
	}

	if (renderFogShadow)
		gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];

	if (!renderFogShadowWater) // set up forward shadows in case they will not be set up below
		rd->FX_SetupShadowsForTransp();

#endif

	if (renderFogShadowWater)
	{
		rd->FX_SetupShadowsForTransp();
		gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
	}

	if (!m_drawWaterSurface && m_pParams->m_viewerInsideVolume)
	{
		// set projection matrix for full screen quad
		rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Push();
		Matrix44A* m = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->GetTop();
		mathMatrixOrthoOffCenterLH(m, -1, 1, -1, 1, -1, 1);
		if (!IsRecursiveRenderView())
		{
			const SRenderTileInfo& rti = rd->GetRenderTileInfo();
			if (rti.nGridSizeX > 1.f || rti.nGridSizeY > 1.f)
			{
				// shift and scale viewport
				m->m00 *= rti.nGridSizeX;
				m->m11 *= rti.nGridSizeY;
				m->m30 = -((rti.nGridSizeX - 1.f) - rti.nPosX * 2.0f);
				m->m31 = ((rti.nGridSizeY - 1.f) - rti.nPosY * 2.0f);
			}
		}

		// set view matrix to identity
		rd->PushMatrix();
		rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

		rd->EF_DirtyMatrix();
	}

	// render
	uint32 nPasses(0);
	ef->FXBegin(&nPasses, 0);
	if (0 == nPasses)
	{
		// reset matrices
		rd->PopMatrix();
		rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Pop();
		rd->EF_DirtyMatrix();
		return false;
	}
	ef->FXBeginPass(0);

	// set ps constants
	if (!m_drawWaterSurface || m_drawFastPath && !m_pParams->m_viewerInsideVolume)
	{
		if (!m_pOceanParams)
		{
			// fog color & density
			const Vec3 col = m_pParams->m_fogColorAffectedBySun ? m_pParams->m_fogColor.CompMul(gEnv->p3DEngine->GetSunColor()) : m_pParams->m_fogColor;
			const Vec4 fogColorDensity(col, 1.44269502f * m_pParams->m_fogDensity);   // log2(e) = 1.44269502
			static CCryNameR Param1Name("cFogColorDensity");
			ef->FXSetPSFloat(Param1Name, &fogColorDensity, 1);
		}
		else
		{
			// fog color & density
			Vec4 fogColorDensity(m_pOceanParams->m_fogColor.CompMul(gEnv->p3DEngine->GetSunColor()), 1.44269502f * m_pOceanParams->m_fogDensity);     // log2(e) = 1.44269502
			static CCryNameR Param1Name("cFogColorDensity");
			ef->FXSetPSFloat(Param1Name, &fogColorDensity, 1);

			// fog color shallow & water level
			Vec4 fogColorShallowWaterLevel(m_pOceanParams->m_fogColorShallow.CompMul(gEnv->p3DEngine->GetSunColor()), -m_pParams->m_fogPlane.d);
			static CCryNameR Param2Name("cFogColorShallowWaterLevel");
			ef->FXSetPSFloat(Param2Name, &fogColorShallowWaterLevel, 1);

			if (m_pParams->m_viewerInsideVolume)
			{
				// under water in-scattering constant term = exp2( -fogDensity * ( waterLevel - cameraPos.z) )
				float c(expf(-m_pOceanParams->m_fogDensity * (-m_pParams->m_fogPlane.d - rd->GetCamera().GetPosition().z)));
				Vec4 underWaterInScatterConst(c, 0, 0, 0);
				static CCryNameR Param3Name("cUnderWaterInScatterConst");
				ef->FXSetPSFloat(Param3Name, &underWaterInScatterConst, 1);
			}
		}

		Vec4 fogPlane(m_pParams->m_fogPlane.n.x, m_pParams->m_fogPlane.n.y, m_pParams->m_fogPlane.n.z, m_pParams->m_fogPlane.d);
		static CCryNameR Param4Name("cFogPlane");
		ef->FXSetPSFloat(Param4Name, &fogPlane, 1);

		if (m_pParams->m_viewerInsideVolume)
		{
			Vec4 perpDist(m_pParams->m_fogPlane | rd->GetRCamera().vOrigin, 0, 0, 0);
			static CCryNameR Param5Name("cPerpDist");
			ef->FXSetPSFloat(Param5Name, &perpDist, 1);
		}
	}

	// Disable fog when inside volume or when not using fast path - could assign custom RT flag for this instead
	if (m_drawFastPath && m_pParams->m_viewerInsideVolume || !m_drawFastPath && m_drawWaterSurface)
	{
		// fog color & density
		const Vec4 fogColorDensity(0, 0, 0, 0);
		static CCryNameR Param1Name("cFogColorDensity");
		ef->FXSetPSFloat(Param1Name, &fogColorDensity, 1);
	}

	{
		static CCryNameR pszParamBBoxMin("vBBoxMin");
		static CCryNameR pszParamBBoxMax("vBBoxMax");
		const Vec4 vBBoxMin(m_pParams->m_WSBBox.min, 1.0f);
		const Vec4 vBBoxMax(m_pParams->m_WSBBox.max, 1.0f);
		ef->FXSetPSFloat(pszParamBBoxMin, &vBBoxMin, 1);
		ef->FXSetPSFloat(pszParamBBoxMax, &vBBoxMax, 1);
	}

	// set vs constants
	Vec4 viewerColorToWaterPlane(m_pParams->m_viewerCloseToWaterPlane && m_pParams->m_viewerCloseToWaterVolume ? 0.0f : 1.0f,
	                             m_pParams->m_viewerCloseToWaterVolume ? 2.0f * 2.0f : 0.0f,
	                             0.0f,
	                             0.0f);
	static CCryNameR Param6Name("cViewerColorToWaterPlane");
	ef->FXSetVSFloat(Param6Name, &viewerColorToWaterPlane, 1);

	if (bCaustics)
	{
		Vec4 pCausticsParams = Vec4(CRenderer::CV_r_watercausticsdistance, m_pParams->m_causticIntensity, m_pParams->m_causticTiling, m_pParams->m_causticHeight);

		static CCryNameR m_pCausticParams("vCausticParams");
		ef->FXSetPSFloat(m_pCausticParams, &pCausticsParams, 1);
	}

	static CCryNameR volFogShadowRangeN("volFogShadowRange");
	ef->FXSetPSFloat(volFogShadowRangeN, &volFogShadowRange, 1);

	if (renderFogShadowWater)
	{
		//set world basis
		float maskRTWidth = float(rd->GetWidth());
		float maskRTHeight = float(rd->GetHeight());

		Vec4 vScreenScale(1.0f / maskRTWidth, 1.0f / maskRTHeight, 0.0f, 0.0f);

		Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
		Vec4 vParamValue, vMag;
		CShadowUtils::ProjectScreenToWorldExpansionBasis(rd->m_IdentityMatrix, rd->GetCamera(), rd->m_vProjMatrixSubPixoffset, maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, true, NULL);

		Vec4 vWorldBasisX = vWBasisX;
		Vec4 vWorldBasisY = vWBasisY;
		Vec4 vWorldBasisZ = vWBasisZ;
		Vec4 vCameraPos = vCamPos;

		static CCryNameR m_pScreenScale("ScreenScale");
		static CCryNameR m_pCamPos("vCamPos");
		static CCryNameR m_pWBasisX("vWBasisX");
		static CCryNameR m_pWBasisY("vWBasisY");
		static CCryNameR m_pWBasisZ("vWBasisZ");

		ef->FXSetPSFloat(m_pScreenScale, &vScreenScale, 1);
		ef->FXSetPSFloat(m_pCamPos, &vCameraPos, 1);
		ef->FXSetPSFloat(m_pWBasisX, &vWorldBasisX, 1);
		ef->FXSetPSFloat(m_pWBasisY, &vWorldBasisY, 1);
		ef->FXSetPSFloat(m_pWBasisZ, &vWorldBasisZ, 1);
	}

#if defined(VOLUMETRIC_FOG_SHADOWS)
	if (renderFogShadow)
	{
		Vec3 volFogShadowDarkeningP;
		gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_DARKENING, volFogShadowDarkeningP);

		Vec4 volFogShadowDarkening(volFogShadowDarkeningP, 0);
		static CCryNameR volFogShadowDarkeningN("volFogShadowDarkening");
		ef->FXSetPSFloat(volFogShadowDarkeningN, &volFogShadowDarkening, 1);

		const float aSun = (1.0f - clamp_tpl(volFogShadowDarkeningP.y, 0.0f, 1.0f)) * 1.0f;
		const float bSun = 1.0f - aSun;
		const float aAmb = (1.0f - clamp_tpl(volFogShadowDarkeningP.z, 0.0f, 1.0f)) * 0.4f;
		const float bAmb = 1.0f - aAmb;

		Vec4 volFogShadowDarkeningSunAmb(aSun, bSun, aAmb, bAmb);
		static CCryNameR volFogShadowDarkeningSunAmbN("volFogShadowDarkeningSunAmb");
		ef->FXSetPSFloat(volFogShadowDarkeningSunAmbN, &volFogShadowDarkeningSunAmb, 1);
	}
#endif

	if (m_drawWaterSurface || !m_pParams->m_viewerInsideVolume)
	{
		// copy vertices into dynamic VB
		TempDynVB<SVF_P3F_C4B_T2F>::CreateFillAndBind(m_pParams->m_pVertices, m_pParams->m_numVertices, 0);

		// copy indices into dynamic IB
		TempDynIB16::CreateFillAndBind(m_pParams->m_pIndices, m_pParams->m_numIndices);

		// set vertex declaration
		if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
		{
			// commit all render changes
			rd->FX_Commit();

			// draw
			ERenderPrimitiveType eType = eptTriangleList;
			if (CHWShader_D3D::s_pCurInstHS)
				eType = ept3ControlPointPatchList;
			rd->FX_DrawIndexedPrimitive(eType, 0, 0, m_pParams->m_numVertices, 0, m_pParams->m_numIndices);
		}
	}
	else
	{
		// NOTE: Get aligned stack-space (pointer and size aligned to manager's alignment requirement)
		CryStackAllocWithSizeVector(SVF_P3F_T3F, 4, pVB, CDeviceBufferManager::AlignBufferSizeForStreaming);

		Vec3 coords[8];
		rd->GetRCamera().CalcVerts(coords);

		pVB[0].p.x = -1;
		pVB[0].p.y = 1;
		pVB[0].p.z = 0.5f;
		pVB[0].st = coords[5] - coords[1];

		pVB[1].p.x = 1;
		pVB[1].p.y = 1;
		pVB[1].p.z = 0.5f;
		pVB[1].st = coords[4] - coords[0];

		pVB[2].p.x = -1;
		pVB[2].p.y = -1;
		pVB[2].p.z = 0.5f;
		pVB[2].st = coords[6] - coords[2];

		pVB[3].p.x = 1;
		pVB[3].p.y = -1;
		pVB[3].p.z = 0.5f;
		pVB[3].st = coords[7] - coords[3];

		TempDynVB<SVF_P3F_T3F>::CreateFillAndBind(pVB, 4, 0);

		// set vertex declaration
		if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
		{
			// commit all render changes
			rd->FX_Commit();
			// draw
			rd->FX_DrawPrimitive(eptTriangleStrip, 0, 4);
		}

		// reset matrices
		rd->PopMatrix();
		rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Pop();
		rd->EF_DirtyMatrix();
	}

	ef->FXEndPass();
	ef->FXEnd();

	gcpRendD3D->m_RP.m_FlagsShader_RT = nFlagsShaderRTSave;

	return true;
}

bool CREWaterVolume::Compile(CRenderObject* pObj)
{
	if (!m_pCompiledObject)
	{
		return false;
	}

	auto& cro = *(m_pCompiledObject);
	cro.m_bValid = 0;

	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;
	SRenderPipeline& rp(rd->m_RP);
	auto nThreadID = rp.m_nProcessThreadID;
	CRY_ASSERT(rd->m_pRT->IsRenderThread());
	auto* pWaterStage = rd->GetGraphicsPipeline().GetWaterStage();

	if (!pWaterStage
	    || !pObj
	    || !pObj->m_pCurrMaterial)
	{
		return false;
	}

	auto& shaderItem = pObj->m_pCurrMaterial->GetShaderItem(0);
	CShader* pShader = (CShader*)shaderItem.m_pShader;
	CShaderResources* RESTRICT_POINTER pResources = static_cast<CShaderResources*>(shaderItem.m_pShaderResources);

	if (!pShader
	    || pShader->m_eShaderType != eST_Water
	    || !pResources
	    || !pResources->m_pCompiledResourceSet)
	{
		return false;
	}

	// create PSOs which match to specific material.
	EVertexFormat vertexFormat = eVF_P3F_C4B_T2F;
	SGraphicsPipelineStateDescription psoDescription(
	  pObj,
	  this,
	  shaderItem,
	  TTYPE_GENERAL, // set as default, this may be overwritten in CreatePipelineStates().
	  vertexFormat,
	  0 /*geomInfo.CalcStreamMask()*/,
	  eptTriangleList // tessellation is handled in CreatePipelineStates(). ept3ControlPointPatchList is used in that case.
	  );

	// TODO: remove this if old graphics pipeline is removed.
	// NOTE: this is to use a typed constant buffer instead of per batch constant buffer.
	psoDescription.objectRuntimeMask |= g_HWSR_MaskBit[HWSR_COMPUTE_SKINNING];

	// fog related runtime mask, this changes eventual PSOs.
	const bool bFog = rp.m_TI[nThreadID].m_FS.m_bEnable;
	const bool bVolumetricFog = (rd->m_bVolumetricFogEnabled != 0);
#if defined(VOLUMETRIC_FOG_SHADOWS)
	const bool bVolFogShadow = (rd->m_bVolFogShadowsEnabled != 0);
#else
	const bool bVolFogShadow = false;
#endif
	psoDescription.objectRuntimeMask |= bFog ? g_HWSR_MaskBit[HWSR_FOG] : 0;
	psoDescription.objectRuntimeMask |= bVolumetricFog ? g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG] : 0;
	psoDescription.objectRuntimeMask |= bVolFogShadow ? g_HWSR_MaskBit[HWSR_SAMPLE5] : 0;

	uint32 passMask = 0;
	bool bFullscreen = false;
	bool bCaustics = false;
	bool bRenderFogShadowWater = false;
	bool bAllowTessellation = false;
	CRY_ASSERT(m_pParams);

	if (m_drawWaterSurface)
	{
		passMask |= CWaterStage::ePassMask_ReflectionGen;
		passMask |= CWaterStage::ePassMask_WaterSurface;
		passMask |= CWaterStage::ePassMask_CausticsGen;

		bFullscreen = false;

		bCaustics = CRenderer::CV_r_watercaustics &&
		            CRenderer::CV_r_watercausticsdeferred &&
		            CRenderer::CV_r_watervolumecaustics &&
		            m_pParams->m_caustics &&
		            (-m_pParams->m_fogPlane.d >= 1.0f); // unfortunately packing to RG8 limits us to heights over 1 meter, so we just disable if volume goes below.

		bAllowTessellation = true;

		// TODO: replace this shader flag with shader constant to avoid shader permutation after removing old graphics pipeline.
		// TODO: remove RBPF2_RAINRIPPLES after removing old graphics pipeline.
		const bool bDeferredRain = rd->m_bDeferredRainEnabled;
		psoDescription.objectRuntimeMask |= bDeferredRain ? g_HWSR_MaskBit[HWSR_OCEAN_PARTICLE] : 0;
	}
	else
	{
		// TODO: replace this shader flag with shader constant to avoid shader permutation after removing old graphics pipeline.
		// forward shadow runtime mask. this changes eventual PSOs.
		// only water fog volume is affected by shadow.
		bRenderFogShadowWater = (CRenderer::CV_r_FogShadowsWater > 0) && (m_pParams->m_fogShadowing > 0.01f);
		psoDescription.objectRuntimeMask |= bRenderFogShadowWater ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;

		// NOTE: PSO caches belong to a material so different material can use different PSO if pass id is same.
		//       For example, different material is used to render inside water fog volume or outside water fog volume.
		passMask |= CWaterStage::ePassMask_FogVolume;

		if (m_pParams->m_viewerInsideVolume)
		{
			bFullscreen = true;
		}
		else // !(m_pParams->m_viewerInsideVolume)
		{
			bFullscreen = false;
		}
	}
	CRY_ASSERT(passMask != 0);

	cro.m_bHasTessellation = 0;
#ifdef WATER_TESSELLATION_RENDERER
	// Enable tessellation for water geometry
	if (bAllowTessellation && (pShader->m_Flags2 & EF2_HW_TESSELLATION))
	{
		cro.m_bHasTessellation = 1;
	}
#endif
	psoDescription.objectFlags |= cro.m_bHasTessellation ? FOB_ALLOW_TESSELLATION : 0;

	if (!pWaterStage->CreatePipelineStates(passMask, cro.m_psoArray, psoDescription, pResources->m_pipelineStateCache.get()))
	{
		if (!CRenderer::CV_r_shadersAllowCompilation)
		{
			return true;
		}
		else
		{
			return false;  // Shaders might still compile; try recompiling object later
		}
	}

	cro.m_pMaterialResourceSet = pResources->m_pCompiledResourceSet;
	cro.m_bCaustics = bCaustics ? 1 : 0;

	cro.m_pPerInstanceResourceSet = pWaterStage->GetDefaultPerInstanceResourceSet();

	UpdateVertex(cro, bFullscreen);

	// UpdatePerInstanceCB uses not thread safe functions like CreateConstantBuffer(),
	// so this needs to be called here instead of DrawToCommandList().
	UpdatePerInstanceCB(cro, *pObj, bRenderFogShadowWater, bCaustics);

	CRY_ASSERT(cro.m_pMaterialResourceSet);
	CRY_ASSERT(cro.m_pPerInstanceCB);
	CRY_ASSERT(cro.m_pPerInstanceResourceSet);
	CRY_ASSERT(cro.m_vertexStreamSet);
	CRY_ASSERT((!bFullscreen && cro.m_indexStreamSet != nullptr) || (bFullscreen && cro.m_indexStreamSet == nullptr));

	// Issue the barriers on the core command-list, which executes directly before the Draw()s in multi-threaded jobs
	PrepareForUse(cro, false, *(CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()));

	cro.m_bValid = 1;
	return true;
}

void CREWaterVolume::DrawToCommandList(CRenderObject* pObj, const struct SGraphicsPipelinePassContext& ctx)
{
	if (!m_pCompiledObject || !(m_pCompiledObject->m_bValid))
		return;

	auto& RESTRICT_REFERENCE cobj = *m_pCompiledObject;

#if defined(ENABLE_PROFILING_CODE)
	if (!cobj.m_bValid || !cobj.m_pMaterialResourceSet->IsValid())
	{
		CryInterlockedIncrement(&SPipeStat::Out()->m_nIncompleteCompiledObjects);
	}
#endif

	CRY_ASSERT(ctx.stageID == eStage_Water);
	const CDeviceGraphicsPSOPtr& pPso = cobj.m_psoArray[ctx.passID];

	if (!pPso || !pPso->IsValid() || !cobj.m_pMaterialResourceSet->IsValid())
		return;

	// Don't render in caustics gen pass unless needed.
	if ((ctx.passID == CWaterStage::ePass_CausticsGen) && !cobj.m_bCaustics)
		return;

	CRY_ASSERT(cobj.m_pPerInstanceCB);
	CRY_ASSERT(cobj.m_pPerInstanceResourceSet && cobj.m_pPerInstanceResourceSet->IsValid());

	CDeviceGraphicsCommandInterface& RESTRICT_REFERENCE commandInterface = *(ctx.pCommandList->GetGraphicsInterface());

	// Set states
	commandInterface.SetPipelineState(pPso.get());
	commandInterface.SetResources(EResourceLayoutSlot_PerMaterialRS, cobj.m_pMaterialResourceSet.get(), EShaderStage_AllWithoutCompute);
	commandInterface.SetResources(EResourceLayoutSlot_PerInstanceExtraRS, cobj.m_pPerInstanceResourceSet.get(), EShaderStage_AllWithoutCompute);

	EShaderStage perInstanceCBShaderStages =
	  cobj.m_bHasTessellation
	  ? (EShaderStage_Domain | EShaderStage_Vertex | EShaderStage_Pixel)
	  : (EShaderStage_Vertex | EShaderStage_Pixel);
	commandInterface.SetInlineConstantBuffer(EResourceLayoutSlot_PerInstanceCB, cobj.m_pPerInstanceCB, eConstantBufferShaderSlot_PerInstance, perInstanceCBShaderStages);

	if (CRenderer::CV_r_NoDraw != 3)
	{
		CRY_ASSERT(cobj.m_vertexStreamSet);
		commandInterface.SetVertexBuffers(1, 0, cobj.m_vertexStreamSet);

		if (cobj.m_indexStreamSet == nullptr)
		{
			commandInterface.Draw(cobj.m_nVerticesCount, 1, 0, 0);
		}
		else
		{
			commandInterface.SetIndexBuffer(cobj.m_indexStreamSet);
			commandInterface.DrawIndexed(cobj.m_nNumIndices, 1, cobj.m_nStartIndex, 0, 0);
		}
	}
}

void CREWaterVolume::PrepareForUse(watervolume::SCompiledWaterVolume& compiledObj, bool bInstanceOnly, CDeviceCommandList& RESTRICT_REFERENCE commandList) const
{
	CDeviceGraphicsCommandInterface* pCommandInterface = commandList.GetGraphicsInterface();

	if (!bInstanceOnly)
	{
		pCommandInterface->PrepareResourcesForUse(EResourceLayoutSlot_PerMaterialRS, compiledObj.m_pMaterialResourceSet.get(), EShaderStage_AllWithoutCompute);
	}

	pCommandInterface->PrepareResourcesForUse(EResourceLayoutSlot_PerInstanceExtraRS, compiledObj.m_pPerInstanceResourceSet.get(), EShaderStage_AllWithoutCompute);

	EShaderStage perInstanceCBShaderStages =
	  compiledObj.m_bHasTessellation
	  ? (EShaderStage_Domain | EShaderStage_Vertex | EShaderStage_Pixel)
	  : (EShaderStage_Vertex | EShaderStage_Pixel);
	pCommandInterface->PrepareInlineConstantBufferForUse(EResourceLayoutSlot_PerInstanceCB, compiledObj.m_pPerInstanceCB, eConstantBufferShaderSlot_PerInstance, perInstanceCBShaderStages);

	{
		if (!bInstanceOnly)
		{
			pCommandInterface->PrepareVertexBuffersForUse(1, 0, compiledObj.m_vertexStreamSet);

			if (compiledObj.m_indexStreamSet == nullptr)
			{
				return;
			}

			pCommandInterface->PrepareIndexBufferForUse(compiledObj.m_indexStreamSet);
		}
	}
}

void CREWaterVolume::UpdatePerInstanceCB(
  watervolume::SCompiledWaterVolume& RESTRICT_REFERENCE compiledObj,
  const CRenderObject& renderObj,
  bool bRenderFogShadowWater,
  bool bCaustics) const
{
	CD3D9Renderer* const RESTRICT_POINTER rd = gcpRendD3D;
	SRenderPipeline& rp(rd->m_RP);
	SCGParamsPF& PF = rd->m_cEF.m_PF[rp.m_nProcessThreadID];
	const auto cameraPos = PF.pCameraPos;

	if (!compiledObj.m_pPerInstanceCB)
	{
		compiledObj.m_pPerInstanceCB = rd->m_DevBufMan.CreateConstantBufferRaw(sizeof(watervolume::SPerInstanceConstantBuffer));
	}

	if (!compiledObj.m_pPerInstanceCB)
	{
		return;
	}

	CryStackAllocWithSize(watervolume::SPerInstanceConstantBuffer, cb, CDeviceBufferManager::AlignBufferSizeForStreaming);

	if (!m_drawWaterSurface && m_pParams->m_viewerInsideVolume)
	{
		Matrix44 m;
		m.SetIdentity();
		cb->PerInstanceWorldMatrix = m;
	}
	else
	{
		cb->PerInstanceWorldMatrix = renderObj.m_II.m_Matrix;
	}

	if (m_CustomData)
	{
		// passed from COcean::Render() and CWaterWaveRenderNode::Render().
		float* pData = static_cast<float*>(m_CustomData);
		cb->OceanParams0 = Vec4(pData[0], pData[1], pData[2], pData[3]);
		cb->OceanParams1 = Vec4(pData[4], pData[5], pData[6], pData[7]);
	}
	else
	{
		cb->OceanParams0 = Vec4(0.0f);
		cb->OceanParams1 = Vec4(0.0f);
	}

	cb->cFogColorDensity = Vec4(0.0f);

	if (!m_drawWaterSurface || m_drawFastPath && !m_pParams->m_viewerInsideVolume)
	{
		const float log2e = 1.44269502f; // log2(e) = 1.44269502

		if (!m_pOceanParams)
		{
			// fog color & density
			const Vec3 col = m_pParams->m_fogColorAffectedBySun ? m_pParams->m_fogColor.CompMul(PF.pSunColor) : m_pParams->m_fogColor;
			cb->cFogColorDensity = Vec4(col, log2e * m_pParams->m_fogDensity);
		}
		else
		{
			// fog color & density
			cb->cFogColorDensity = Vec4(m_pOceanParams->m_fogColor.CompMul(PF.pSunColor), log2e * m_pOceanParams->m_fogDensity);

			// fog color shallow & water level
			cb->cFogColorShallowWaterLevel = Vec4(m_pOceanParams->m_fogColorShallow.CompMul(PF.pSunColor), -m_pParams->m_fogPlane.d);
			if (m_pParams->m_viewerInsideVolume)
			{
				// under water in-scattering constant term = exp2( -fogDensity * ( waterLevel - cameraPos.z) )
				float c(expf(-m_pOceanParams->m_fogDensity * (-m_pParams->m_fogPlane.d - cameraPos.z)));
				cb->cUnderWaterInScatterConst = Vec4(c, 0.0f, 0.0f, 0.0f);
			}
		}

		cb->cFogPlane = Vec4(m_pParams->m_fogPlane.n.x, m_pParams->m_fogPlane.n.y, m_pParams->m_fogPlane.n.z, m_pParams->m_fogPlane.d);
		if (m_pParams->m_viewerInsideVolume)
		{
			cb->cPerpDist = Vec4(m_pParams->m_fogPlane | cameraPos, 0.0f, 0.0f, 0.0f);
		}
	}

	// Disable fog when inside volume or when not using fast path - could assign custom RT flag for this instead
	if (m_drawFastPath && m_pParams->m_viewerInsideVolume || !m_drawFastPath && m_drawWaterSurface)
	{
		// fog color & density
		cb->cFogColorDensity = Vec4(0.0f);
	}

	if (m_drawWaterSurface)
	{
		cb->vBBoxMin = Vec4(m_pParams->m_WSBBox.min, 1.0f);
		cb->vBBoxMax = Vec4(m_pParams->m_WSBBox.max, 1.0f);
	}
	else
	{
		cb->vBBoxMin = Vec4(0.0f);
		cb->vBBoxMax = Vec4(0.0f);
	}

	Vec4 viewerColorToWaterPlane(m_pParams->m_viewerCloseToWaterPlane && m_pParams->m_viewerCloseToWaterVolume ? 0.0f : 1.0f,
	                             m_pParams->m_viewerCloseToWaterVolume ? 2.0f * 2.0f : 0.0f,
	                             0.0f,
	                             0.0f);
	cb->cViewerColorToWaterPlane = viewerColorToWaterPlane;

	if (bRenderFogShadowWater)
	{
		Vec3 volFogShadowRangeP;
		gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_RANGE, volFogShadowRangeP);
		volFogShadowRangeP.x = clamp_tpl(volFogShadowRangeP.x, 0.01f, 1.0f);
		cb->volFogShadowRange.x = volFogShadowRangeP.x;
		cb->volFogShadowRange.y = volFogShadowRangeP.y;
		cb->volFogShadowRange.z = clamp_tpl(m_pParams->m_fogShadowing, 0.0f, 1.0f);
		cb->volFogShadowRange.w = 1.0f - clamp_tpl(m_pParams->m_fogShadowing, 0.0f, 1.0f);
	}
	else
	{
		cb->volFogShadowRange = Vec4(0.0f);
	}

	if (bCaustics)
	{
		cb->vCausticParams = Vec4(CRenderer::CV_r_watercausticsdistance, m_pParams->m_causticIntensity, m_pParams->m_causticTiling, m_pParams->m_causticHeight);
	}
	else
	{
		cb->vCausticParams = Vec4(0.0f);
	}

	compiledObj.m_pPerInstanceCB->UpdateBuffer(cb, cbSize);
}

void CREWaterVolume::UpdateVertex(watervolume::SCompiledWaterVolume& compiledObj, bool bFullscreen)
{
	CRendElementBase::SGeometryInfo geomInfo;
	ZeroStruct(geomInfo);

	const bool bDrawSurface = (m_drawWaterSurface || !m_pParams->m_viewerInsideVolume);
	CRY_ASSERT((bFullscreen && !bDrawSurface) || (!bFullscreen && bDrawSurface));

	if (!bFullscreen)
	{
		const size_t vertexBufferStride = sizeof(SVF_P3F_C4B_T2F);
		const size_t vertexBufferSize = m_pParams->m_numVertices * vertexBufferStride;
		const size_t indexBufferStride = 2;
		const size_t indexBufferSize = m_pParams->m_numIndices * indexBufferStride;

		// create buffers if needed.
		if (m_vertexBuffer.handle == watervolume::invalidBufferHandle
		    || m_vertexBuffer.size != vertexBufferSize)
		{
			if (m_vertexBuffer.handle != watervolume::invalidBufferHandle)
			{
				gRenDev->m_DevBufMan.Destroy(m_vertexBuffer.handle);
			}
			m_vertexBuffer.handle = gRenDev->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, vertexBufferSize);
			m_vertexBuffer.size = vertexBufferSize;
		}
		if (m_indexBuffer.handle == watervolume::invalidBufferHandle
		    || m_indexBuffer.size != indexBufferSize)
		{
			if (m_indexBuffer.handle != watervolume::invalidBufferHandle)
			{
				gRenDev->m_DevBufMan.Destroy(m_indexBuffer.handle);
			}
			m_indexBuffer.handle = gRenDev->m_DevBufMan.Create(BBT_INDEX_BUFFER, BU_DYNAMIC, indexBufferSize);
			m_indexBuffer.size = indexBufferSize;
		}

		// TODO: update only when water volume surface changes.
		CRY_ASSERT(m_vertexBuffer.handle != watervolume::invalidBufferHandle);
		CRY_ASSERT(gRenDev->m_DevBufMan.Size(m_vertexBuffer.handle) >= vertexBufferSize);
		if (m_vertexBuffer.handle != watervolume::invalidBufferHandle)
		{
			gRenDev->m_DevBufMan.UpdateBuffer(m_vertexBuffer.handle, m_pParams->m_pVertices, vertexBufferSize);
		}

		CRY_ASSERT(m_indexBuffer.handle != watervolume::invalidBufferHandle);
		CRY_ASSERT(gRenDev->m_DevBufMan.Size(m_indexBuffer.handle) >= indexBufferSize);
		if (m_indexBuffer.handle != watervolume::invalidBufferHandle)
		{
			gRenDev->m_DevBufMan.UpdateBuffer(m_indexBuffer.handle, m_pParams->m_pIndices, indexBufferSize);
		}

		const bool bTessellation = compiledObj.m_bHasTessellation;

		// fill geomInfo.
		geomInfo.primitiveType = bTessellation ? ept3ControlPointPatchList : eptTriangleList;
		geomInfo.eVertFormat = eVF_P3F_C4B_T2F;
		geomInfo.nFirstIndex = 0;
		geomInfo.nNumIndices = m_pParams->m_numIndices;
		geomInfo.nFirstVertex = 0;
		geomInfo.nNumVertices = m_pParams->m_numVertices;
		geomInfo.nNumVertexStreams = 1;
		geomInfo.indexStream.hStream = m_indexBuffer.handle;
		geomInfo.indexStream.nStride = (indexBufferStride == 2) ? RenderIndexType::Index16 : RenderIndexType::Index32;
		geomInfo.vertexStreams[0].hStream = m_vertexBuffer.handle;
		geomInfo.vertexStreams[0].nStride = vertexBufferStride;
		geomInfo.vertexStreams[0].nSlot = 0;
		geomInfo.nTessellationPatchIDOffset = 0;
	}
	else
	{
		const size_t vertexNum = 3;
		const size_t vertexBufferStride = sizeof(SVF_P3F_C4B_T2F);
		const size_t vertexBufferSize = vertexNum * vertexBufferStride;

		// create buffers if needed.
		if (m_vertexBuffer.handle == watervolume::invalidBufferHandle
		    || m_vertexBuffer.size != vertexBufferSize)
		{
			if (m_vertexBuffer.handle != watervolume::invalidBufferHandle)
			{
				gRenDev->m_DevBufMan.Destroy(m_vertexBuffer.handle);
			}
			m_vertexBuffer.handle = gRenDev->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_DYNAMIC, vertexBufferSize);
			m_vertexBuffer.size = vertexBufferSize;

			// update vertex buffer once and keep it.
			{
				// NOTE: Get aligned stack-space (pointer and size aligned to manager's alignment requirement)
				CryStackAllocWithSizeVector(SVF_P3F_C4B_T2F, vertexNum, fullscreenTriVertices, CDeviceBufferManager::AlignBufferSizeForStreaming);

				SPostEffectsUtils::GetFullScreenTri(fullscreenTriVertices, 0, 0, 0.0f);

				if (m_vertexBuffer.handle != watervolume::invalidBufferHandle)
				{
					gRenDev->m_DevBufMan.UpdateBuffer(m_vertexBuffer.handle, fullscreenTriVertices, vertexBufferSize);
				}
			}
		}

		CRY_ASSERT(m_vertexBuffer.handle != watervolume::invalidBufferHandle);
		CRY_ASSERT(gRenDev->m_DevBufMan.Size(m_vertexBuffer.handle) >= vertexBufferSize);

		// fill geomInfo.
		geomInfo.primitiveType = eptTriangleStrip;
		geomInfo.eVertFormat = eVF_P3F_C4B_T2F;
		geomInfo.nFirstVertex = 0;
		geomInfo.nNumVertices = vertexNum;
		geomInfo.nNumVertexStreams = 1;
		geomInfo.vertexStreams[0].hStream = m_vertexBuffer.handle;
		geomInfo.vertexStreams[0].nStride = vertexBufferStride;
		geomInfo.vertexStreams[0].nSlot = 0;
		geomInfo.nTessellationPatchIDOffset = 0;
	}

	// Fill stream pointers.
	if (geomInfo.indexStream.hStream != 0)
	{
		compiledObj.m_indexStreamSet = CCryDeviceWrapper::GetObjectFactory().CreateIndexStreamSet(&geomInfo.indexStream);
	}
	else
	{
		compiledObj.m_indexStreamSet = nullptr;
	}
	compiledObj.m_vertexStreamSet = CCryDeviceWrapper::GetObjectFactory().CreateVertexStreamSet(geomInfo.nNumVertexStreams, &geomInfo.vertexStreams[0]);
	compiledObj.m_nNumIndices = geomInfo.nNumIndices;
	compiledObj.m_nStartIndex = geomInfo.nFirstIndex;
	compiledObj.m_nVerticesCount = geomInfo.nNumVertices;
}
