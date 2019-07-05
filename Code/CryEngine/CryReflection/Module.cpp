// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Module.h"

#include <CryCore/Platform/platform_impl.inl>

namespace Cry {
namespace Reflection {

CRYREGISTER_SINGLETON_CLASS(CModule);

CModule* CModule::s_pInstance = nullptr;

CModule::CModule()
{

}

CModule& CModule::GetInstance()
{
	CRY_ASSERT(s_pInstance, "Reflection not yet initialized.");
	return *s_pInstance;
}

bool CModule::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	s_pInstance = this;
	env.pReflection = this;

	return true;
}

} // ~Reflection namespace
} // ~Cry namespace
