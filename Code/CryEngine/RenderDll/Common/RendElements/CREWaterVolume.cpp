// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/CREWaterVolume.h>
#include "XRenderD3D9/DriverD3D.h"

CREWaterVolume::CREWaterVolume()
	: CRendElementBase()
	, m_pParams(0)
	, m_pOceanParams(0)
	, m_drawWaterSurface(false)
	, m_drawFastPath(false)
{
	mfSetType(eDATA_WaterVolume);
	mfUpdateFlags(FCEF_TRANSFORM);
}

CREWaterVolume::~CREWaterVolume()
{
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

	IF(ef->m_eShaderType != eST_Water, 0)
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