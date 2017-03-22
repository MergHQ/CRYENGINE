// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ISystem.h>

struct IAutoCleanup
{
	virtual ~IAutoCleanup()
	{
#if (defined(_LAUNCHER) && defined(CRY_IS_MONOLITHIC_BUILD)) || !defined(_LIB)
		if (gEnv)
		{
			if (auto pConsole = gEnv->pConsole)
			{
				// Unregister all commands that were registered from within the plugin/module
				for (auto& it : g_moduleCommands)
				{
					pConsole->RemoveCommand(it);
				}
				g_moduleCommands.clear();

				// Unregister all CVars that were registered from within the plugin/module
				for (auto& it : g_moduleCVars)
				{
					pConsole->UnregisterVariable(it);
				}
				g_moduleCVars.clear();
			}
		}
#endif
	}
};
