// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CREGpuParticle.h"
#include "GpuParticleComponentRuntime.h"

namespace gpu_pfx2
{

CREGpuParticle::CREGpuParticle() { m_pRuntime = nullptr; }

void CREGpuParticle::mfPrepare(bool bCheckOverflow)
{
	CRenderer* rd = gRenDev;
	SRenderPipeline& rRP = rd->m_RP;

	rd->m_RP.m_CurVFormat = eVF_Unknown;

	rd->m_RP.m_pRE = this;
	rd->m_RP.m_RendNumIndices = m_pRuntime->GetNumParticles() * 4;

	if (rRP.m_lightVolumeBuffer.HasVolumes() && (rRP.m_pCurObject->m_ObjFlags & FOB_LIGHTVOLUME) != 0)
	{
		SRenderObjData* pOD = rRP.m_pCurObject->GetObjData();
#ifndef _RELEASE
		const uint numVolumes = rRP.m_lightVolumeBuffer.GetNumVolumes();
		if (((int32)pOD->m_LightVolumeId - 1) < 0 || ((int32)pOD->m_LightVolumeId - 1) >= numVolumes)
			CRY_ASSERT_MESSAGE(0, "Bad LightVolumeId");
#endif
		rd->RT_SetLightVolumeShaderFlags();
	}
}

bool CREGpuParticle::mfDraw(CShader* ef, SShaderPass* sfm)
{
	PROFILE_LABEL_SCOPE("PFX DRAW PARTICLES");
	CD3D9Renderer* rd(gcpRendD3D);
	SRenderPipeline& rp(rd->m_RP);
	gcpRendD3D->FX_Commit();

	if (m_pRuntime->BindVertexShaderResources())
	{
		// Set vertex-buffer to empty eVF_Unknown
		const uint maxNumSprites = rp.m_particleBuffer.GetMaxNumSprites();
		const uint totalNumSprites = m_pRuntime->GetNumParticles();
		gcpRendD3D->FX_SetVStream(0, nullptr, 0, 3 * sizeof(float));
		rp.m_particleBuffer.BindSpriteIB();
		rp.m_lightVolumeBuffer.BindSRVs();

		for (;; )
		{
			const uint numSprites = min(totalNumSprites, maxNumSprites);
			rd->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, 0, 0, numSprites * 6);
			rp.m_RendNumVerts -= numSprites;
			rp.m_FirstVertex += numSprites;
			if (rp.m_RendNumVerts <= 0)
				break;
			CHWShader_D3D* pVertexShader = (CHWShader_D3D*)sfm->m_VShader;
			pVertexShader->mfSetParametersPB();
			rd->FX_Commit();
		}

		gcpRendD3D->FX_Commit();

		ID3D11ShaderResourceView* ppSRVsNull[1] = { nullptr };
		gcpRendD3D->m_DevMan.BindSRV(CDeviceManager::TYPE_VS, ppSRVsNull, 7, 1);
	}

	return true;
}
}
