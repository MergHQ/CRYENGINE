// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Environment.h"
#include "Common/IImpl.h"

namespace CryAudio
{
CEnvironment::~CEnvironment()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);

	for (auto const pConnection : m_connections)
	{
		g_pIImpl->DestructEnvironmentConnection(pConnection);
	}
}
} // namespace CryAudio