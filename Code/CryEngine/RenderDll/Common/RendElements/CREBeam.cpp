// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/RendElement.h>
#include "Common/RenderView.h"
#include "XRenderD3D9/DriverD3D.h"

void CREBeam::mfPrepare(bool bCheckOverflow)
{
	CRenderer* rd = gRenDev;

	if (bCheckOverflow)
		rd->FX_CheckOverflow(0, 0, this);

	CRenderObject* obj = rd->m_RP.m_pCurObject;

	if (CRenderer::CV_r_beams == 0)
	{
		rd->m_RP.m_pRE = NULL;
		rd->m_RP.m_RendNumIndices = 0;
		rd->m_RP.m_RendNumVerts = 0;
	}
	else
	{
		const int nThreadID = rd->m_RP.m_nProcessThreadID;
		SRenderObjData* pOD = obj->GetObjData();
		SRenderLight* pLight = &rd->m_RP.m_pCurrentRenderView->GetLight(eDLT_DeferredLight, pOD->m_nLightID);
		if (pLight && pLight->m_Flags & DLF_PROJECT)
		{
			rd->m_RP.m_pRE = this;
			rd->m_RP.m_RendNumIndices = 0;
			rd->m_RP.m_RendNumVerts = 0;
		}
		else
		{
			rd->m_RP.m_pRE = NULL;
			rd->m_RP.m_RendNumIndices = 0;
			rd->m_RP.m_RendNumVerts = 0;
		}
	}

}

bool CREBeam::mfCompile(CParserBin& Parser, SParserFrame& Frame)
{
	return true;
}

void CREBeam::SetupGeometry(SVF_P3F_C4B_T2F* pVertices, uint16* pIndices, float fAngleCoeff, float fNear, float fFar)
{
	const int nNumSides = BEAM_RE_CONE_SIDES;
	Vec2 rotations[nNumSides];
	float fIncrement = 1.0f / (float)nNumSides;
	float fAngle = 0.0f;
	for (uint32 i = 0; i < nNumSides; i++)
	{
		sincos_tpl(fAngle, &rotations[i].x, &rotations[i].y);
		fAngle += fIncrement * gf_PI2;
	}

	float fScaleNear = fNear * fAngleCoeff;
	float fScaleFar = fFar * fAngleCoeff;

	UCol cBlack, cWhite;
	cBlack.dcolor = 0;
	cWhite.dcolor = 0xFFFFFFFF;

	for (uint32 i = 0; i < nNumSides; i++) //Near Verts
	{
		pVertices[i].xyz = Vec3(fNear, rotations[i].x * fScaleNear, rotations[i].y * fScaleNear);
		pVertices[i].color = cWhite;
		pVertices[i].st = Vec2(rotations[i].x, rotations[i].y);
	}

	for (uint32 i = 0; i < (nNumSides); i++) // Far verts
	{
		pVertices[i + nNumSides].xyz = Vec3(fFar, rotations[i].x * fScaleFar, rotations[i].y * fScaleFar);
		pVertices[i + nNumSides].color = cWhite;
		pVertices[i + nNumSides].st = Vec2(rotations[i].x, rotations[i].y);
	}

	uint32 nNearCapVert = nNumSides * 2;
	uint32 nFarCapVert = nNumSides * 2 + 1;

	//near cap vert
	pVertices[nNearCapVert].xyz = Vec3(fNear, 0.0f, 0.0f);
	pVertices[nNearCapVert].color = cBlack;
	pVertices[nNearCapVert].st = Vec2(0, 0);

	//far cap vert
	pVertices[nFarCapVert].xyz = Vec3(fFar, 0.0f, 0.0f);
	pVertices[nFarCapVert].color = cWhite;
	pVertices[nFarCapVert].st = Vec2(0, 0);

	for (uint32 i = 0; i < nNumSides; i++)
	{
		uint32 idx = i * 6;
		pIndices[idx] = (i) % (nNumSides);
		pIndices[idx + 1] = (i) % (nNumSides) + nNumSides;
		pIndices[idx + 2] = (i + 1) % (nNumSides) + nNumSides;

		pIndices[idx + 3] = (i + 1) % (nNumSides) + nNumSides;
		pIndices[idx + 4] = (i + 1) % (nNumSides);
		pIndices[idx + 5] = (i) % (nNumSides);
	}

	for (uint32 i = 0; i < nNumSides; i++) // cap plane near
	{
		uint32 idx = ((nNumSides) * 6) + (i * 3);
		pIndices[idx] = nNearCapVert;
		pIndices[idx + 1] = (i) % (nNumSides);
		pIndices[idx + 2] = (i + 1) % (nNumSides);
	}

	for (uint32 i = 0; i < nNumSides; i++) // cap plane far
	{
		uint32 idx = ((nNumSides) * 9) + (i * 3);
		pIndices[idx] = nFarCapVert;
		pIndices[idx + 1] = (i + 1) % (nNumSides) + nNumSides;
		pIndices[idx + 2] = (i) % (nNumSides) + nNumSides;
	}
}

bool CREBeam::mfDraw(CShader* ef, SShaderPass* sl)
{
	CD3D9Renderer* rd = gcpRendD3D;
	int nThreadID = rd->m_RP.m_nProcessThreadID;

	if (IsRecursiveRenderView())
		return false;

	PROFILE_LABEL_SCOPE("LIGHT BEAM");

	EShaderQuality nShaderQuality = (EShaderQuality)gcpRendD3D->EF_GetShaderQuality(eST_FX);
	ERenderQuality nRenderQuality = gRenDev->m_RP.m_eQuality;
	bool bLowSpecShafts = (nShaderQuality == eSQ_Low) || (nRenderQuality == eRQ_Low);

	STexState pState(FILTER_BILINEAR, true);
	const int texStateID(CTexture::GetTexState(pState));

	STexState pStatePoint(FILTER_POINT, true);
	const int texStateIDPoint(CTexture::GetTexState(pStatePoint));

	bool bViewerInsideCone = false;

	CTexture* pLowResRT = CTexture::s_ptexZTargetScaled2; // CTexture::s_ptexBackBufferScaled[1];
	CTexture* pLowResRTDepth = CTexture::s_ptexDepthBufferQuarter;

	SDepthTexture* pCurrDepthSurf = NULL;

#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO || CRY_PLATFORM_APPLE || CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID // Depth surface nastiness
	if (CTexture::IsTextureExist(pLowResRTDepth))
	{
		SDepthTexture D3dDepthSurface;
		D3dDepthSurface.nWidth = pLowResRTDepth->GetWidth();
		D3dDepthSurface.nHeight = pLowResRTDepth->GetHeight();
		D3dDepthSurface.nFrameAccess = -1;
		D3dDepthSurface.bBusy = false;

		D3dDepthSurface.pTexture = pLowResRTDepth;
		D3dDepthSurface.pSurface = pLowResRTDepth->GetDeviceDepthStencilView();
		D3dDepthSurface.pTarget = pLowResRTDepth->GetDevTexture()->Get2DTexture();

		pCurrDepthSurf = &D3dDepthSurface;
	}
#endif

	CRenderObject* pObj = rd->m_RP.m_pCurObject;
	SRenderObjData* pOD = pObj->GetObjData();
	uint16 nLightID = pOD->m_nLightID;
	SRenderLight* pDL = &rd->m_RP.m_pCurrentRenderView->GetLight(eDLT_DeferredLight, pOD->m_nLightID);

	bool bCastsShadows = ((pDL->m_Flags & (DLF_CASTSHADOW_MAPS | DLF_PROJECT)) == (DLF_CASTSHADOW_MAPS | DLF_PROJECT)) ? true : false;

	const CRenderObject::SInstanceInfo& rInstInfo = pObj->m_II;

	Matrix34A objMatInv = rInstInfo.m_Matrix.GetInverted();

	Matrix44A mLightProj, mLightView;
	CShadowUtils::GetCubemapFrustumForLight(pDL, 0, pDL->m_fLightFrustumAngle * 2.f, &mLightProj, &mLightView, true);

	Matrix44 projMat = mLightView * mLightProj;

	const CRenderCamera& RCam = gRenDev->GetRCamera();

	float fLightAngle = pDL->m_fLightFrustumAngle;
	float fAngleCoeff = 1.0f / tan_tpl((90.0f - fLightAngle) * gf_PI / 180.0f);
	float fNear = pDL->m_fProjectorNearPlane;
	float fFar = pDL->m_fRadius;
	float fScaleNear = fNear * fAngleCoeff;
	float fScaleFar = fFar * fAngleCoeff;
	Vec3 vLightPos = pDL->m_Origin;
	Vec3 vAxis = rInstInfo.m_Matrix.GetColumn0();

	float fSin, fCos;
	sincos_tpl(fLightAngle * gf_PI / 180.0f, &fSin, &fCos);

	Vec4 vLightParams = Vec4(fFar, fAngleCoeff, fNear, fFar);
	Vec4 vSphereParams = Vec4(vLightPos, fFar);
	Vec4 vConeParams = Vec4(vAxis, fCos);
	Vec4 pLightPos = Vec4(vLightPos, 1.0f);
	Vec4 cLightDiffuse = Vec4(pDL->m_Color.r, pDL->m_Color.g, pDL->m_Color.b, pDL->m_SpecMult);

	Vec3 vEye = RCam.vOrigin;

	Vec3 vCoords[9];  // Evaluate campos to near plane verts as a sphere.
	RCam.CalcVerts(vCoords);
	vCoords[4] = vEye;
	AABB camExtents(vCoords, 5);

	float fRadius = camExtents.GetRadius();
	Vec3 vCentre = camExtents.GetCenter();

	float fCosSq = fCos * fCos;

	Vec3 vVertToSphere = vCentre - vLightPos;
	Vec3 d = vVertToSphere + vAxis * (fRadius / fSin);
	float dSq = d.dot(d);
	float e = d.dot(vAxis);
	float eSq = e * e;

	if ((e > 0.0f) && (eSq >= (dSq * fCosSq)))
	{
		float fSinSq = fSin * fSin;
		dSq = vVertToSphere.dot(vVertToSphere);
		e = vVertToSphere.dot(vAxis);

		if ((e < (fFar + fRadius)) && (e > (fNear - fRadius))) // test capping planes
		{
			bViewerInsideCone = true;
		}
	}

	Vec4 cEyePosVec(vEye, !bViewerInsideCone ? 1.f : 0);

	Vec4 vShadowCoords = Vec4(0.0f, 0.0f, 1.0f, 1.0f);

	CTexture* pShadowTex = NULL;
	CTexture* pProjTex = NULL;

	if (bCastsShadows)
	{
		const ShadowMapFrustum& shadowFrustum = CShadowUtils::GetFirstFrustum(rd->m_RP.m_pCurrentRenderView, nLightID);
		if (shadowFrustum.bUseShadowsPool)
		{
			pShadowTex = CTexture::s_ptexRT_ShadowPool;
			float fTexWidth = (float)CTexture::s_ptexRT_ShadowPool->GetWidth();
			float fTexHeight = (float)CTexture::s_ptexRT_ShadowPool->GetHeight();
			vShadowCoords = Vec4(shadowFrustum.packX[0] / fTexWidth, shadowFrustum.packY[0] / fTexHeight, shadowFrustum.packWidth[0] / fTexWidth, shadowFrustum.packHeight[0] / fTexHeight);
		}
	}

	if (pDL->m_pLightImage)
	{
		pProjTex = (CTexture*)pDL->m_pLightImage;
	}

	Vec4 sampleOffsets[5];
	{
		const float tU = 1.0f / (float)pLowResRT->GetWidth();
		const float tV = 1.0f / (float)pLowResRT->GetHeight();

		sampleOffsets[0] = Vec4(0, 0, 0, 0);
		sampleOffsets[1] = Vec4(0, -tV, tU, tV);
		sampleOffsets[2] = Vec4(-tU, 0, -tU, tV);
		sampleOffsets[3] = Vec4(tU, 0, tU, -tV);
		sampleOffsets[4] = Vec4(0, tV, -tU, -tV);
	}

	Vec4 vMisc = Vec4(1.0f / (float)gcpRendD3D->m_nShadowPoolWidth, 1.0f / (float)gcpRendD3D->m_nShadowPoolHeight, 0.0f, 0.0f);

	const int ZPass = 2; // passes can be buggy, use manual ordering
	const int VolumetricPass = 1;
	const int FinalPass = 0;

	rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);

	if (bCastsShadows)
	{
		rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
	}

	//Setup geometry
	const int nNumSides = BEAM_RE_CONE_SIDES;
	const uint32 c_numBBVertices(nNumSides * 2 + 2);
	SVF_P3F_C4B_T2F bbVertices[c_numBBVertices];

	const uint32 c_numBBIndices((nNumSides) * 6 * 2);
	uint16 bbIndices[c_numBBIndices];

	SetupGeometry(&bbVertices[0], &bbIndices[0], fAngleCoeff, fNear, fFar);

	// copy vertices into dynamic VB
	TempDynVB<SVF_P3F_C4B_T2F>::CreateFillAndBind(bbVertices, c_numBBVertices, 0);

	// copy indices into dynamic IB
	TempDynIB16::CreateFillAndBind(bbIndices, c_numBBIndices);

	uint32 nPasses = 0;
	ef->FXBegin(&nPasses, FEF_DONTSETSTATES);

	assert(nPasses == (ZPass + 1));

	int nStartPass = (bViewerInsideCone || !pCurrDepthSurf) ? VolumetricPass : ZPass;

	for (int nCurPass = nStartPass; nCurPass > -1; nCurPass--)
	{
		ef->FXBeginPass(nCurPass);

		//set world basis
		float maskRTWidthL = (float)pLowResRT->GetWidth();
		float maskRTHeightL = (float)pLowResRT->GetHeight();
		float maskRTWidthH = (float)rd->GetWidth();
		float maskRTHeightH = (float)rd->GetHeight();
		Vec4 vScreenScale(1.0f / maskRTWidthL, 1.0f / maskRTHeightL, 1.0f / maskRTWidthH, 1.0f / maskRTHeightH);

		if (nCurPass == nStartPass && pLowResRT)
		{
			rd->FX_ClearTarget(pLowResRT, Clr_Transparent);
			rd->FX_ClearTarget(pCurrDepthSurf, CLEAR_ZBUFFER);
			rd->FX_PushRenderTarget(0, pLowResRT, pCurrDepthSurf, -1, false, 1);
		}

		uint32 nState = (nCurPass == FinalPass) ? (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA) : 0;

		if (bViewerInsideCone)
		{
			rd->SetCullMode(R_CULL_FRONT);
		}
		else
		{
			rd->SetCullMode(R_CULL_BACK);
		}

		if (bViewerInsideCone || !pCurrDepthSurf)
		{
			nState |= GS_NODEPTHTEST;
		}
		else
		{
			nState |= (nCurPass == ZPass) ? (GS_DEPTHWRITE | GS_COLMASK_NONE) : 0;
		}

		rd->FX_SetState(nState);

		// set vs constants
		if (nCurPass == VolumetricPass)
		{
			ef->FXSetVSFloat(m_eyePosInWSName, &cEyePosVec, 1);

			ef->FXSetPSFloat(m_eyePosInWSName, &cEyePosVec, 1);
			ef->FXSetPSFloat(m_projMatrixName, (const Vec4*)&projMat.m00, 4);
			ef->FXSetPSFloat(m_shadowCoordsName, (const Vec4*)&vShadowCoords, 1);
			ef->FXSetPSFloat(m_lightParamsName, &vLightParams, 1);
			ef->FXSetPSFloat(m_sphereParamsName, &vSphereParams, 1);
			ef->FXSetPSFloat(m_coneParamsName, &vConeParams, 1);
			ef->FXSetPSFloat(m_lightPosName, &pLightPos, 1);
			ef->FXSetPSFloat(m_miscOffsetsName, &vMisc, 1);
		}
		else if (nCurPass == FinalPass)
		{
			ef->FXSetPSFloat(m_sampleOffsetsName, &sampleOffsets[0], 5);
		}

		ef->FXSetPSFloat(m_lightDiffuseName, &cLightDiffuse, 1);
		ef->FXSetPSFloat(m_screenScaleName, &vScreenScale, 1);

		if (nCurPass == FinalPass && pLowResRT)
			pLowResRT->Apply(7, texStateID);
		if (pProjTex)
			pProjTex->Apply(5, texStateID);
		if (bCastsShadows && pShadowTex)
			pShadowTex->Apply(6, texStateID); // bilinear is a hack, but looks better
																				//pShadowTex->Apply(6, texStateIDPoint);

		rd->m_RP.m_nCommitFlags |= FC_MATERIAL_PARAMS;

		// commit all render changes
		rd->FX_Commit();

		// set vertex declaration and streams of skydome
		if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
		{
			// draw skydome
			rd->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, c_numBBVertices, 0, c_numBBIndices);
		}

		if ((nCurPass == VolumetricPass) && pLowResRT)
		{
			rd->FX_PopRenderTarget(0);
		}

	}

	return true;
}
