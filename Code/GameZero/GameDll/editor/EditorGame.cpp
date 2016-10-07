// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorGame.h"

extern "C"
{
	GAME_API IGameStartup* CreateGameStartup();
};

void CEditorGame::SetPlayerPosAng(Vec3 pos, Vec3 viewDir)
{
	if (auto* pPlayerEntity = gEnv->pGameFramework->GetClientEntity())
	{
		pPlayerEntity->SetPosRotScale(pos, Quat::CreateRotationVDir(viewDir), Vec3(1.0f), ENTITY_XFORM_EDITOR);
	}
}