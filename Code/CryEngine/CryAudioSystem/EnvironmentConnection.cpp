// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EnvironmentConnection.h"
#include "Common.h"
#include "Common/IImpl.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CEnvironmentConnection::~CEnvironmentConnection()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);
	g_pIImpl->DestructEnvironment(m_pImplData);
}
} // namespace CryAudio
