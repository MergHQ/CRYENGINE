// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SnapshotConnection.h"

namespace ACE
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CSnapshotConnection::Serialize(Serialization::IArchive& ar)
{
	EActionType const actionType = m_actionType;
	int const changeoverTime = m_changeoverTime;

	ar(m_actionType, "action", "Action");
	ar(m_changeoverTime, "time", "Changeover Time (ms)");
	m_changeoverTime = std::max(0, m_changeoverTime);

	if (ar.isInput())
	{
		if ((actionType != m_actionType) || (changeoverTime != m_changeoverTime))
		{
			SignalConnectionChanged();
		}
	}
}

SERIALIZATION_ENUM_BEGIN_NESTED(CSnapshotConnection, EActionType, "Action Type")
SERIALIZATION_ENUM(CSnapshotConnection::EActionType::Start, "start", "Start")
SERIALIZATION_ENUM(CSnapshotConnection::EActionType::Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()
} // namespace Adx2
} // namespace Impl
} // namespace ACE
