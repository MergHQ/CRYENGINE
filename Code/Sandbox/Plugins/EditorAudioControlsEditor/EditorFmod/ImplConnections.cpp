// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ImplConnections.h"

#include <CrySerialization/Enum.h>
#include <CrySerialization/Decorators/Range.h>

namespace ACE
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
void CEventConnection::Serialize(Serialization::IArchive& ar)
{
	EEventType const type = m_type;

	ar(m_type, "action", "Action");

	if (ar.isInput())
	{
		if (type != m_type)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSnapshotConnection::Serialize(Serialization::IArchive& ar)
{
	ESnapshotType const type = m_type;

	ar(m_type, "action", "Action");

	if (ar.isInput())
	{
		if (type != m_type)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CParamConnection::Serialize(Serialization::IArchive& ar)
{
	float const mult = m_mult;
	float const shift = m_shift;

	ar(m_mult, "mult", "Multiply");
	ar(m_shift, "shift", "Shift");

	if (ar.isInput())
	{
		if ((mult != m_mult) || (shift != m_shift))
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CParamToStateConnection::Serialize(Serialization::IArchive& ar)
{
	float const value = m_value;

	if (m_type == EImplItemType::Parameter)
	{
		ar(m_value, "value", "Value");
	}
	else if (m_type == EImplItemType::VCA)
	{
		ar(Serialization::Range<float>(m_value, 0.0f, 1.0f), "value", "Value");
		m_value = crymath::clamp(m_value, 0.0f, 1.0f);
	}

	if (ar.isInput())
	{
		if (value != m_value)
		{
			SignalConnectionChanged();
		}
	}
}

SERIALIZATION_ENUM_BEGIN(EEventType, "Event Type")
SERIALIZATION_ENUM(EEventType::Start, "start", "Start")
SERIALIZATION_ENUM(EEventType::Stop, "stop", "Stop")
SERIALIZATION_ENUM(EEventType::Pause, "pause", "Pause")
SERIALIZATION_ENUM(EEventType::Resume, "resume", "Resume")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN(ESnapshotType, "Snapshot Type")
SERIALIZATION_ENUM(ESnapshotType::Start, "start", "Start")
SERIALIZATION_ENUM(ESnapshotType::Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()
} // namespace Fmod
} // namespace ACE
