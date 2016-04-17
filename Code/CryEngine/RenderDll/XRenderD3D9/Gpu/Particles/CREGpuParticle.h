// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryRenderer/IGpuParticles.h>

namespace gpu_pfx2
{

class CParticleComponentRuntime;

class CREGpuParticle : public CRendElementBase
{
public:
	CREGpuParticle();
	virtual void mfPrepare(bool bCheckOverflow);
	virtual bool mfDraw(CShader* ef, SShaderPass* sfm);
	void         SetRuntime(CParticleComponentRuntime* pRuntime) { m_pRuntime = pRuntime; };
private:
	CParticleComponentRuntime* m_pRuntime;
};
}
