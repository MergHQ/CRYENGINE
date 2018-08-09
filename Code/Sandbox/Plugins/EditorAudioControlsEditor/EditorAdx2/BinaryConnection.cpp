// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "BinaryConnection.h"

#include "Impl.h"

namespace ACE
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CBinaryConnection::Serialize(Serialization::IArchive& ar)
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
void CBinaryConnection::SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled)
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
bool CBinaryConnection::IsPlatformEnabled(PlatformIndexType const platformIndex) const
{
	return (m_configurationsMask & (1 << platformIndex)) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CBinaryConnection::ClearPlatforms()
{
	m_configurationsMask = 0;
}
} // namespace Adx2
} // namespace Impl
} // namespace ACE
