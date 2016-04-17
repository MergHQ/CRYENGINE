// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "Player.h"


CPlayer::CPlayer()
{
}

CPlayer::~CPlayer()
{
}

bool CPlayer::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);
	return pGameObject->BindToNetwork();
}

void CPlayer::PostInit(IGameObject* pGameObject)
{
	m_extensions.push_back("ViewExtension");
	m_extensions.push_back("InputExtension");
	m_extensions.push_back("MovementExtension");

	for (auto& extension : m_extensions)
	{
		pGameObject->AcquireExtension(extension.c_str());
	}
}

void CPlayer::Release()
{
	for (auto& extension : m_extensions)
	{
		GetGameObject()->ReleaseExtension(extension.c_str());
	}

	ISimpleExtension::Release();
}
