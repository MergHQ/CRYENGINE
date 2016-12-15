// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Player.h"

CPlayer::CPlayer()
{
}

bool CPlayer::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);
	return pGameObject->BindToNetwork();
}

void CPlayer::PostInit(IGameObject* pGameObject)
{
	pGameObject->AcquireExtension("ViewExtension");
	pGameObject->AcquireExtension("InputExtension");
	pGameObject->AcquireExtension("MovementExtension");
}
