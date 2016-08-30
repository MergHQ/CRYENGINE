// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "game/Game.h"
#include "startup/GameStartup.h"
#include "editor/EditorGame.h"
#include <CryCore/Platform/CryLibrary.h>


extern "C"
{
	GAME_API IGameStartup* CreateGameStartup()
	{
		return CGameStartup::Create();
	}

	GAME_API IEditorGame* CreateEditorGame()
	{
		return new CEditorGame();
	}
}

#if !defined(_LIB)

static HMODULE s_frameworkDLL;

static void CleanupFrameworkDLL()
{
	assert( s_frameworkDLL );
	CryFreeLibrary( s_frameworkDLL );
	s_frameworkDLL = 0;
}

HMODULE GetFrameworkDLL(const char* binariesDir)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Load %s", GAME_FRAMEWORK_FILENAME);
	if (!s_frameworkDLL)
	{
		if (binariesDir && binariesDir[0])
		{
			string dllName = PathUtil::Make(binariesDir, GAME_FRAMEWORK_FILENAME);
			s_frameworkDLL = CryLoadLibrary(dllName.c_str());		
		}
		else
		{
			s_frameworkDLL = CryLoadLibrary(GAME_FRAMEWORK_FILENAME);
		}
		atexit( CleanupFrameworkDLL );
	}
	return s_frameworkDLL;
}

#endif
