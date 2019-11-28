// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: Game DLL entry point.

   -------------------------------------------------------------------------
   History:
   - 2:8:2004   10:38 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameStartup.h"
#include "EditorGame.h"

#include <CryCore/Platform/CryLibrary.h>

extern "C"
{
	DLL_EXPORT IGame* CreateGame(IGameFramework* pGameFramework)
	{
		static char pGameBuffer[sizeof(CGame)];
		return new((void*)pGameBuffer)CGame();
	}

	DLL_EXPORT IGameStartup* CreateGameStartup()
	{
		return CGameStartup::Create();
	}
	DLL_EXPORT IEditorGame* CreateEditorGame()
	{
		return new CEditorGame();
	}
}
