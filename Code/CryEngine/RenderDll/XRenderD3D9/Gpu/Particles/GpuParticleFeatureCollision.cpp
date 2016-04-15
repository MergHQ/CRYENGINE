// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuParticleFeatureCollision.h"
#include "GpuParticleComponentRuntime.h"

namespace gpu_pfx2
{

void CFeatureCollision::Update(const gpu_pfx2::SUpdateContext& context)
{
	CParticleComponentRuntime* pRuntime = (CParticleComponentRuntime*) context.pRuntime;

	CTexture* zTarget = CTexture::s_ptexZTarget;
	pRuntime->SetUpdateSRV(eFeatureUpdateSrvSlot_depthBuffer, (ID3D11ShaderResourceView*) zTarget->GetShaderResourceView());
	pRuntime->SetUpdateFlags(EFeatureUpdateFlags_Collision_ScreenSpace);
}

void CFeatureCollision::InternalSetParameters(const EParameterType type, const SFeatureParametersBase& p)
{

}

}
