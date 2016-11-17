// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "STDEnv.h"

#include <CryExtension/CryCreateClassInstance.h>

#include "AutoRegister.h"
#include "Entity/EntityObjectClassRegistry.h"
#include "Entity/EntityObjectDebugger.h"
#include "Entity/EntityObjectMap.h"
#include "Utils/SystemStateMonitor.h"

namespace Schematyc
{
namespace
{
inline bool WantUpdate()
{
	return !gEnv->pGameFramework->IsGamePaused() && (gEnv->pSystem->GetSystemGlobalState() == ESYSTEM_GLOBAL_STATE_RUNNING);
}
}

class CSchematycSTDEnvCreator : public IGameFrameworkExtensionCreator
{
	CRYINTERFACE_SIMPLE(IGameFrameworkExtensionCreator)

	CRYGENERATE_SINGLETONCLASS(CSchematycSTDEnvCreator, ms_szClassName, 0x1a7a070565d7463f, 0xba69b3083263e778)

public:

	virtual ICryUnknown* Create(IGameFramework* pIGameFramework, void* pData) override
	{
		// Create Schematyc standard environment.
		std::shared_ptr<CSTDEnv> pSchematycSTDEnv;
		if (CryCreateClassInstance(CSTDEnv::ms_szClassName, pSchematycSTDEnv))
		{
			// Register Schematyc standard environment with game framework
			pIGameFramework->RegisterExtension(pSchematycSTDEnv);
			// Ensure registration was successful and cache local pointer.
			GetSchematycSTDEnv();
			return pSchematycSTDEnv.get();
		}
		return nullptr;
	}

public:

	static const char* ms_szClassName;
};

CRYREGISTER_SINGLETON_CLASS(CSchematycSTDEnvCreator)

CSchematycSTDEnvCreator::CSchematycSTDEnvCreator() {}

CSchematycSTDEnvCreator::~CSchematycSTDEnvCreator() {}

const char* CSchematycSTDEnvCreator::ms_szClassName = "GameExtension::SchematycSTDEnvCreator";

ISTDEnv* CreateSTDEnv(IGameFramework* pIGameFramework)
{
	IGameFrameworkExtensionCreatorPtr pCreator;
	if (CryCreateClassInstance(CSchematycSTDEnvCreator::ms_szClassName, pCreator))
	{
		return cryinterface_cast<ISTDEnv>(pCreator->Create(pIGameFramework, nullptr));
	}
	return nullptr;
}

CRYREGISTER_CLASS(CSTDEnv)

CSTDEnv::CSTDEnv()
	: m_pSystemStateMonitor(new CSystemStateMonitor())
	, m_pEntityObjectClassRegistry(new CEntityObjectClassRegistry())
	, m_pEntityObjectMap(new CEntityObjectMap())
	, m_pEntityObjectDebugger(new CEntityObjectDebugger())
{
	// #SchematycTODO : Shouldn't log recording, loading and compiling be handled automatically by the Schematyc core?

	ICore& core = GetSchematycCore();

	if (gEnv->IsEditor())
	{
		core.GetLogRecorder().Begin();
	}

	m_pEntityObjectClassRegistry->Init();

	GetSchematycCore().GetEnvRegistry().RegisterPackage(SCHEMATYC_MAKE_ENV_PACKAGE("e2e023df-afa7-43a6-bad4-1bc04eada8e7"_schematyc_guid, "STDEnv", Delegate::Make(*this, &CSTDEnv::RegisterPackage)));

	if (gEnv->IsEditor())
	{
		core.GetLogRecorder().End();
	}

	core.RefreshLogFileSettings();
}

CSTDEnv::~CSTDEnv() {}

const CSystemStateMonitor& CSTDEnv::GetSystemStateMonitor() const
{
	SCHEMATYC_ENV_ASSERT_FATAL(m_pSystemStateMonitor);
	return *m_pSystemStateMonitor;
}

CEntityObjectClassRegistry& CSTDEnv::GetEntityObjectClassRegistry()
{
	SCHEMATYC_ENV_ASSERT_FATAL(m_pEntityObjectClassRegistry);
	return *m_pEntityObjectClassRegistry;
}

CEntityObjectMap& CSTDEnv::GetEntityObjectMap()
{
	SCHEMATYC_ENV_ASSERT_FATAL(m_pEntityObjectMap);
	return *m_pEntityObjectMap;
}

CEntityObjectDebugger& CSTDEnv::GetEntityObjectDebugger()
{
	SCHEMATYC_ENV_ASSERT_FATAL(m_pEntityObjectDebugger);
	return *m_pEntityObjectDebugger;
}

void CSTDEnv::RegisterPackage(IEnvRegistrar& registrar)
{
	CAutoRegistrar::Process(registrar);
}

const char* CSTDEnv::ms_szClassName = "GameExtension::SchematycSTDEnv";
} // Schematyc
