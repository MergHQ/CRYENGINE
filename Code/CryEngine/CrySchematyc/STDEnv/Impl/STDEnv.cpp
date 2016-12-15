// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "STDEnv.h"

#include <CryCore/Platform/platform_impl.inl>
#include <CryGame/IGameFramework.h>

#include "AutoRegister.h"
#include "Entity/EntityObjectClassRegistry.h"
#include "Entity/EntityObjectDebugger.h"
#include "Entity/EntityObjectMap.h"
#include "Utils/SystemStateMonitor.h"

#include "Schematyc/ICore.h"

namespace Schematyc
{
namespace
{
inline bool WantUpdate()
{
	return !gEnv->pGameFramework->IsGamePaused() && (gEnv->pSystem->GetSystemGlobalState() == ESYSTEM_GLOBAL_STATE_RUNNING);
}
} // Anonymous

CSTDEnv::CSTDEnv()
	: m_pSystemStateMonitor(new CSystemStateMonitor())
	, m_pEntityObjectClassRegistry(new CEntityObjectClassRegistry())
	, m_pEntityObjectMap(new CEntityObjectMap())
	, m_pEntityObjectDebugger(new CEntityObjectDebugger())
{}

CSTDEnv::~CSTDEnv()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	s_pInstance = nullptr;
}

const char* CSTDEnv::GetName() const
{
	return "SchematycSTDEnv";
}

const char* CSTDEnv::GetCategory() const
{
	return "Plugin";
}

bool CSTDEnv::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	SCHEMATYC_CORE_ASSERT(!s_pInstance);

	s_pInstance = this;

	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	return true;
}

void CSTDEnv::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	if (event == ESYSTEM_EVENT_GAME_POST_INIT)
	{
		// #SchematycTODO : Shouldn't log recording, loading and compiling be handled automatically by the Schematyc core?

		if (gEnv->IsEditor())
		{
			gEnv->pSchematyc->GetLogRecorder().Begin();
		}

		m_pEntityObjectClassRegistry->Init();
		gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(SCHEMATYC_MAKE_ENV_PACKAGE("e2e023df-afa7-43a6-bad4-1bc04eada8e7"_schematyc_guid, "STDEnv", Delegate::Make(*this, &CSTDEnv::RegisterPackage)));

		if (gEnv->IsEditor())
		{
			gEnv->pSchematyc->GetLogRecorder().End();
		}

		gEnv->pSchematyc->RefreshLogFileSettings();
	}
}

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

CSTDEnv& CSTDEnv::GetInstance()
{
	SCHEMATYC_CORE_ASSERT(s_pInstance);
	return *s_pInstance;
}

void CSTDEnv::RegisterPackage(IEnvRegistrar& registrar)
{
	CAutoRegistrar::Process(registrar);
}

CSTDEnv* CSTDEnv::s_pInstance = nullptr;

CRYREGISTER_SINGLETON_CLASS(CSTDEnv)
} // Schematyc
