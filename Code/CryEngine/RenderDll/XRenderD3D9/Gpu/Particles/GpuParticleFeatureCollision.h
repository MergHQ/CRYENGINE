// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     23/10/2015 by Benjamin Block
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GpuParticleFeatureBase.h"

namespace gpu_pfx2
{
struct CFeatureCollision : public CFeature
{
	static const EGpuFeatureType type = eGpuFeatureType_Collision;

	virtual void Update(const gpu_pfx2::SUpdateContext& context) override;
	virtual void InternalSetParameters(const EParameterType type, const SFeatureParametersBase& p) override;

private:
};
}
