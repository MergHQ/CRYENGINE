// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CrySystem/ConsoleRegistration.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		int           SCvars::debugDrawZTestOn;
		float         SCvars::debugDrawLineThickness;

		void SCvars::Register()
		{
			REGISTER_CVAR2("udr_debugDrawZTestOn", &debugDrawZTestOn, 1, 0,
				"0/1: Disable/enable z-tests for 3D debug geometry.");

			REGISTER_CVAR2("udr_debugDrawLineThickness", &debugDrawLineThickness, 1.0f, 0,
				"Thickness of all 3d lines that are used for debug drawing.");
		}

		void SCvars::Unregister()
		{
			gEnv->pConsole->UnregisterVariable("udr_debugDrawZTestOn");
			gEnv->pConsole->UnregisterVariable("udr_debugDrawLineThickness");
		}

	}
}