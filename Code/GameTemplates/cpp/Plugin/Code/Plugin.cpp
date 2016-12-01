#include "StdAfx.h"
#include "Plugin.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

IEntityRegistrator *IEntityRegistrator::g_pFirst = nullptr;
IEntityRegistrator *IEntityRegistrator::g_pLast = nullptr;

CPlugin::~CPlugin()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

bool CPlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	return true;
}

void CPlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_GAME_POST_INIT:
	{
		// Register entities
		if (IEntityRegistrator::g_pFirst != nullptr)
		{
			do
			{
				IEntityRegistrator::g_pFirst->Register();

				IEntityRegistrator::g_pFirst = IEntityRegistrator::g_pFirst->m_pNext;
			} while (IEntityRegistrator::g_pFirst != nullptr);
		}
	}
	break;
	}
}

CRYREGISTER_SINGLETON_CLASS(CPlugin)