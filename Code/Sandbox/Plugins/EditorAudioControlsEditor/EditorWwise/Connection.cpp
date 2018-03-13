// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Connection.h"

#include "EditorImpl.h"

namespace ACE
{
namespace Wwise
{
static float const s_precision = 0.0001f;

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

	ar(m_value, "value", "Value");

	if (ar.isInput())
	{
		if (fabs(value - m_value) > s_precision)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBankConnection::Serialize(Serialization::IArchive& ar)
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
void CSoundBankConnection::SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled)
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
bool CSoundBankConnection::IsPlatformEnabled(PlatformIndexType const platformIndex) const
{
	return (m_configurationsMask & (1 << platformIndex)) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CSoundBankConnection::ClearPlatforms()
{
	m_configurationsMask = 0;
}
} // namespace Wwise
} // namespace ACE
