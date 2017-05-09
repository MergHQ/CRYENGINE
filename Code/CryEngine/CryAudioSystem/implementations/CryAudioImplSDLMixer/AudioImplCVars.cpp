// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImplCVars.h"
#include <CrySystem/IConsole.h>

using namespace CryAudio::Impl::SDL_mixer;

//////////////////////////////////////////////////////////////////////////
void CCVars::RegisterVariables()
{
}

//////////////////////////////////////////////////////////////////////////
void CCVars::UnregisterVariables()
{
	IConsole* const pConsole = gEnv->pConsole;

	if (pConsole != nullptr)
	{
	}
}
