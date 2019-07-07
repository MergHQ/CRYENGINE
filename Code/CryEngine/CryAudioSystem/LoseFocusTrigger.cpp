// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "LoseFocusTrigger.h"
#include "TriggerUtils.h"
#include "Common/IImpl.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CLoseFocusTrigger::~CLoseFocusTrigger()
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during %s", __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////
void CLoseFocusTrigger::Execute() const
{
	if (!m_connections.empty())
	{
		ExecuteDefaultTriggerConnections(this, m_connections);
	}
	else
	{
		g_pIImpl->OnLoseFocus();
	}
}

//////////////////////////////////////////////////////////////////////////
void CLoseFocusTrigger::AddConnections(TriggerConnections const& connections)
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during %s", __FUNCTION__);
	m_connections = connections;
}

//////////////////////////////////////////////////////////////////////////
void CLoseFocusTrigger::Clear()
{
	for (auto const pConnection : m_connections)
	{
		g_pIImpl->DestructTriggerConnection(pConnection);
	}

	m_connections.clear();
}
} // namespace CryAudio
