// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Setting.h"
#include "Common/IImpl.h"
#include "Common/ISettingConnection.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CSetting::~CSetting()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);

	for (auto const pConnection : m_connections)
	{
		g_pIImpl->DestructSettingConnection(pConnection);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSetting::Load() const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->Load();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSetting::Unload() const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->Unload();
	}
}
} // namespace CryAudio
