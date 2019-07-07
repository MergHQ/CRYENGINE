// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CueConnection.h"

#include <CrySerialization/Enum.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CCueConnection::Serialize(Serialization::IArchive& ar)
{
	EActionType const actionType = m_actionType;

	ar(m_actionType, "action", "Action");

	if (ar.isInput())
	{
		if (actionType != m_actionType)
		{
			SignalConnectionChanged();
		}
	}
}

SERIALIZATION_ENUM_BEGIN_NESTED(CCueConnection, EActionType, "Action Type")
SERIALIZATION_ENUM(CCueConnection::EActionType::Start, "start", "Start")
SERIALIZATION_ENUM(CCueConnection::EActionType::Stop, "stop", "Stop")
SERIALIZATION_ENUM(CCueConnection::EActionType::Pause, "pause", "Pause")
SERIALIZATION_ENUM(CCueConnection::EActionType::Resume, "resume", "Resume")
SERIALIZATION_ENUM_END()
} // namespace Adx2
} // namespace Impl
} // namespace ACE
