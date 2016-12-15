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
}