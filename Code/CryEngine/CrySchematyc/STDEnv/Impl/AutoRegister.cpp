// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AutoRegister.h"

namespace Schematyc
{
CAutoRegistrar::CAutoRegistrar(CallbackPtr pCallback)
	: m_pCallback(pCallback)
{}

void CAutoRegistrar::Process(IEnvRegistrar& registrar)
{
	for (const CAutoRegistrar* pInstance = GetFirstInstance(); pInstance; pInstance = pInstance->GetNextInstance())
	{
		(*pInstance->m_pCallback)(registrar);
	}
}
} // Schematyc
