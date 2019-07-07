// Copyright 2001-2019 Crytek GmbH. All rights reserved.

#include "StdAfx.h"

#include "CryLink/IFramework.h"
#include "CryLinkCVars.h"

#include <CrySystem/IConsole.h>

#if CRY_PLATFORM_WINDOWS
	#include <windows.h>
#endif

namespace CryLinkService
{
CVars CVars::m_instance;

CVars::CVars()
	: cryLinkAllowedClient(nullptr)
	, cryLinkServicePassword(nullptr)
	, cryLinkEditorServicePort(0)
	, cryLinkGameServicePort(0)
	, m_bRegistered(false)
{}

CVars::~CVars()
{
	//Unregister();
}

void CVars::Register()
{
	if (!m_bRegistered && gEnv)
	{
		IConsole* pConsole = gEnv->pConsole;
		CRY_ASSERT(pConsole);
		if (pConsole)
		{
			char computerName[MAX_COMPUTERNAME_LENGTH + 1] = { "" };
			DWORD computerNameSize = sizeof(computerName);
#if CRY_PLATFORM_WINDOWS
			::GetComputerName(computerName, &computerNameSize);
#endif

			cryLinkAllowedClient = REGISTER_STRING(STRINGIFY(cryLinkAllowedClient), computerName, VF_DEV_ONLY, "Host that is allowed to use the CryLink service.");
			cryLinkServicePassword = REGISTER_STRING(STRINGIFY(cryLinkServicePassword), "crylink_pw", VF_DEV_ONLY, "Password needed to establish a connection to CryLink service.");

			REGISTER_CVAR(cryLinkEditorServicePort, 1880, VF_DEV_ONLY, "Port the CryLink service uses in editor. Must be between 1024-32767.");
			//REGISTER_CVAR(cryLinkGameServicePort, 1880, VF_DEV_ONLY, "Port the CryLink service uses in game. Must be between 1024-32767.");

			m_bRegistered = true;
		}
	}
}

void CVars::Unregister()
{
	if (m_bRegistered && gEnv)
	{
		IConsole* pConsole = gEnv->pConsole;
		CRY_ASSERT(pConsole);
		if (pConsole)
		{
			pConsole->UnregisterVariable(STRINGIFY(cryLinkAllowedClient), true);
			pConsole->UnregisterVariable(STRINGIFY(cryLinkServicePassword), true);
			pConsole->UnregisterVariable(STRINGIFY(cryLinkEditorServicePort), true);
			//pConsole->UnregisterVariable(STRINGIFY(cryLinkGameServicePort), true);

			m_bRegistered = false;
		}
	}
}
}
