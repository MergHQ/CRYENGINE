// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/CREFogVolume.h>
#include "XRenderD3D9/DriverD3D.h"

CREFogVolume::CREFogVolume()
	: CRendElementBase()
	, m_center(0.0f, 0.0f, 0.0f)
	, m_viewerInsideVolume(0)
	, m_stencilRef(0)
	, m_reserved(0)
	, m_localAABB(Vec3(-1, -1, -1), Vec3(1, 1, 1))
	, m_matWSInv()
	, m_fogColor(1, 1, 1, 1)
	, m_globalDensity(1)
	, m_softEdgesLerp(1, 0)
	, m_heightFallOffDirScaled(0, 0, 1)
	, m_heightFallOffBasePoint(0, 0, 0)
	, m_eyePosInWS(0, 0, 0)
	, m_eyePosInOS(0, 0, 0)
	, m_rampParams(0, 0, 0)
	, m_windOffset(0, 0, 0)
	, m_noiseScale(0)
	, m_noiseFreq(1, 1, 1)
	, m_noiseOffset(0)
	, m_noiseElapsedTime(0)
	, m_emission(0, 0, 0)
{
	mfSetType(eDATA_FogVolume);
	mfUpdateFlags(FCEF_TRANSFORM);

	m_matWSInv.SetIdentity();
}

CREFogVolume::~CREFogVolume()
{
}

void CREFogVolume::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

bool CREFogVolume::mfDraw(CShader* ef, SShaderPass* sfm)
{
	CD3D9Renderer* rd(gcpRendD3D);

#if !defined(_RELEASE)
	if (ef->m_eShaderType != eST_PostProcess)
	{
		CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Incorrect shader set for fog volume");
		return false;
	}
#endif

	PROFILE_LABEL_SCOPE("FOG_VOLUME");

	rd->m_RP.m_PersFlags2 |= RBPF2_MSAA_SAMPLEFREQ_PASS;
	rd->FX_SetMSAAFlagsRT();

	// render
	uint32 nPasses(0);
	ef->FXBegin(&nPasses, 0);
	if (0 == nPasses)
	{
		assert(0);
		rd->m_RP.m_PersFlags2 &= ~RBPF2_MSAA_SAMPLEFREQ_PASS;
		return(false);
	}
	ef->FXBeginPass(0);

	if (m_viewerInsideVolume)
	{
		rd->SetCullMode(R_CULL_FRONT);
		int nState = GS_COLMASK_RGB | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
		nState |= m_nearCutoff ? 0 : GS_NODEPTHTEST;
		rd->FX_SetState(nState);
	}
	else
	{
		rd->SetCullMode(R_CULL_BACK);
		rd->FX_SetState(GS_COLMASK_RGB | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
	}

	// set vs constants
	static CCryNameR invObjSpaceMatrixName("invObjSpaceMatrix");
	ef->FXSetVSFloat(invObjSpaceMatrixName, (const Vec4*)&m_matWSInv.m00, 3);

	const Vec4 cNearCutoffVec(m_nearCutoff, (!m_viewerInsideVolume ? 1.0f : 0.0f), 0.0f, 0.0f);
	static CCryNameR nearCutoffName("nearCutoff");
	ef->FXSetVSFloat(nearCutoffName, &cNearCutoffVec, 1);

	// set ps constants
	const Vec4 cFogColVec(m_fogColor.r, m_fogColor.g, m_fogColor.b, 0);
	static CCryNameR fogColorName("fogColor");
	ef->FXSetPSFloat(fogColorName, &cFogColVec, 1);

	const Vec4 cGlobalDensityVec(m_globalDensity, 1.44269502f * m_globalDensity, 0, 0);
	static CCryNameR globalDensityName("globalDensity");
	ef->FXSetPSFloat(globalDensityName, &cGlobalDensityVec, 1);

	const Vec4 cDensityOffsetVec(m_densityOffset, m_densityOffset, m_densityOffset, m_densityOffset);
	static CCryNameR densityOffsetName("densityOffset");
	ef->FXSetPSFloat(densityOffsetName, &cDensityOffsetVec, 1);

	const Vec4 cHeigthFallOffBasePointVec(m_heightFallOffBasePoint, 0);
	static CCryNameR heightFallOffBasePointName("heightFallOffBasePoint");
	ef->FXSetPSFloat(heightFallOffBasePointName, &cHeigthFallOffBasePointVec, 1);

	const Vec4 cHeightFallOffDirScaledVec(m_heightFallOffDirScaled, 0);
	static CCryNameR heightFallOffDirScaledName("heightFallOffDirScaled");
	ef->FXSetPSFloat(heightFallOffDirScaledName, &cHeightFallOffDirScaledVec, 1);

	const Vec4 cOutsideSoftEdgesLerpVec(m_softEdgesLerp.x, m_softEdgesLerp.y, 0, 0);
	static CCryNameR outsideSoftEdgesLerpName("outsideSoftEdgesLerp");
	ef->FXSetPSFloat(outsideSoftEdgesLerpName, &cOutsideSoftEdgesLerpVec, 1);

	// commit all render changes
	rd->FX_Commit();

	// set vertex declaration and streams of skydome
	if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
	{
		// define bounding box geometry
		const uint32 c_numBBVertices(8);
		SVF_P3F_C4B_T2F bbVertices[c_numBBVertices] =
		{
			{ Vec3(m_localAABB.min.x, m_localAABB.min.y, m_localAABB.min.z) },
			{ Vec3(m_localAABB.min.x, m_localAABB.max.y, m_localAABB.min.z) },
			{ Vec3(m_localAABB.max.x, m_localAABB.max.y, m_localAABB.min.z) },
			{ Vec3(m_localAABB.max.x, m_localAABB.min.y, m_localAABB.min.z) },
			{ Vec3(m_localAABB.min.x, m_localAABB.min.y, m_localAABB.max.z) },
			{ Vec3(m_localAABB.min.x, m_localAABB.max.y, m_localAABB.max.z) },
			{ Vec3(m_localAABB.max.x, m_localAABB.max.y, m_localAABB.max.z) },
			{ Vec3(m_localAABB.max.x, m_localAABB.min.y, m_localAABB.max.z) }
		};

		const uint32 c_numBBIndices(36);
		static const uint16 bbIndices[c_numBBIndices] =
		{
			0, 1, 2, 0, 2, 3,
			7, 6, 5, 7, 5, 4,
			3, 2, 6, 3, 6, 7,
			4, 5, 1, 4, 1, 0,
			1, 5, 6, 1, 6, 2,
			4, 0, 3, 4, 3, 7
		};

		// copy vertices into dynamic VB
		TempDynVB<SVF_P3F_C4B_T2F>::CreateFillAndBind(bbVertices, c_numBBVertices, 0);

		// copy indices into dynamic IB
		TempDynIB16::CreateFillAndBind(bbIndices, c_numBBIndices);
		// draw skydome
		rd->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, c_numBBVertices, 0, c_numBBIndices);
	}

	ef->FXEndPass();
	ef->FXEnd();

	rd->m_RP.m_PersFlags2 &= ~RBPF2_MSAA_SAMPLEFREQ_PASS;

	return(true);
}