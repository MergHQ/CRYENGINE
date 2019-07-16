// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Module.h"

#include <CryCore/Platform/platform_impl.inl>
#include "ScaleformHelper.h"

namespace Cry {
namespace ScaleformModule {

CRYREGISTER_SINGLETON_CLASS(CModule);

CModule* CModule::s_pInstance = nullptr;

CModule::CModule()
{
}

CModule& CModule::GetInstance()
{
	CRY_ASSERT_MESSAGE(s_pInstance, "Scaleform is not initialized yet.");
	return *s_pInstance;
}

bool CModule::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	s_pInstance = this;
	env.pScaleformHelper = new Cry::Scaleform4::CScaleformHelper;

	return true;
}

} // ~ScaleformModule namespace
} // ~Cry namespace
