// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Connection.h"

#include "EditorImpl.h"

#include <CrySerialization/Enum.h>
#include <CrySerialization/Decorators/Range.h>

namespace ACE
{
namespace Fmod
{
static float const s_precision = 0.0001f;

//////////////////////////////////////////////////////////////////////////
void CEventConnection::Serialize(Serialization::IArchive& ar)
{
	EEventActionType const actionType = m_actionType;

	ar(m_actionType, "action", "Action");

	if (ar.isInput())
	{
		if (actionType != m_actionType)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSnapshotConnection::Serialize(Serialization::IArchive& ar)
{
	ESnapshotActionType const actionType = m_actionType;

	ar(m_actionType, "action", "Action");

	if (ar.isInput())
	{
		if (actionType != m_actionType)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Serialize(Serialization::IArchive& ar)
{
	float const mult = m_mult;
	float const shift = m_shift;

	ar(m_mult, "mult", "Multiply");
	ar(m_shift, "shift", "Shift");

	if (ar.isInput())
	{
		if (fabs(mult - m_mult) > s_precision ||
		    fabs(shift - m_shift) > s_precision)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameterToStateConnection::Serialize(Serialization::IArchive& ar)
{
	float const value = m_value;

	if (m_itemType == EItemType::Parameter)
	{
		ar(m_value, "value", "Value");
	}
	else if (m_itemType == EItemType::VCA)
	{
		ar(Serialization::Range(m_value, 0.0f, 1.0f), "value", "Value");
		m_value = crymath::clamp(m_value, 0.0f, 1.0f);
	}

	if (ar.isInput())
	{
		if (fabs(value - m_value) > s_precision)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBankConnection::Serialize(Serialization::IArchive& ar)
{
	PlatformIndexType const configurationsMask = m_configurationsMask;
	size_t const numPlatforms = s_platforms.size();

	for (size_t i = 0; i < numPlatforms; ++i)
	{
		auto const platformIndex = static_cast<PlatformIndexType>(i);
		bool isEnabled = IsPlatformEnabled(platformIndex);
		ar(isEnabled, s_platforms[i], s_platforms[i]);
		SetPlatformEnabled(platformIndex, isEnabled);
	}

	if (ar.isInput())
	{
		if (configurationsMask != m_configurationsMask)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBankConnection::SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled)
{
	if (isEnabled)
	{
		m_configurationsMask |= 1 << platformIndex;
	}
	else
	{
		m_configurationsMask &= ~(1 << platformIndex);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBankConnection::IsPlatformEnabled(PlatformIndexType const platformIndex) const
{
	return (m_configurationsMask & (1 << platformIndex)) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CBankConnection::ClearPlatforms()
{
	m_configurationsMask = 0;
}

SERIALIZATION_ENUM_BEGIN(EEventActionType, "Event Action Type")
SERIALIZATION_ENUM(EEventActionType::Start, "start", "Start")
SERIALIZATION_ENUM(EEventActionType::Stop, "stop", "Stop")
SERIALIZATION_ENUM(EEventActionType::Pause, "pause", "Pause")
SERIALIZATION_ENUM(EEventActionType::Resume, "resume", "Resume")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN(ESnapshotActionType, "Snapshot Action Type")
SERIALIZATION_ENUM(ESnapshotActionType::Start, "start", "Start")
SERIALIZATION_ENUM(ESnapshotActionType::Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()
} // namespace Fmod
} // namespace ACE

