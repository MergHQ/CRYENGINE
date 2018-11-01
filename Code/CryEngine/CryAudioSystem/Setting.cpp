// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Setting.h"
#include "SettingConnection.h"
#include "Common/ISetting.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CSetting::~CSetting()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSetting::Load() const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->GetImplData()->Load();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSetting::Unload() const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->GetImplData()->Unload();
	}
}
} // namespace CryAudio
