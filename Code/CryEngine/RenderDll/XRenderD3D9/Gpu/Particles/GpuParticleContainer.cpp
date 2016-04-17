// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuParticleContainer.h"

namespace gpu_pfx2
{
void CParticleContainer::Initialize(bool isDoubleBuffered)
{
	m_defaultData.Initialize(isDoubleBuffered);
	m_counter.CreateDeviceBuffer();
}

void CParticleContainer::ReadbackCounter()
{
	m_counter.Readback();
}

int CParticleContainer::RetrieveCounter()
{
	int result = 0;
	if (const uint* counter = m_counter.Map())
	{
		result = counter[0];
		m_counter.Unmap();
	}
	return result;
}

}
