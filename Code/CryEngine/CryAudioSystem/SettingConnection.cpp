// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SettingConnection.h"
#include "Common.h"
#include "Common/IImpl.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CSettingConnection::~CSettingConnection()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);
	g_pIImpl->DestructSetting(m_pImplData);
}
} // namespace CryAudio
