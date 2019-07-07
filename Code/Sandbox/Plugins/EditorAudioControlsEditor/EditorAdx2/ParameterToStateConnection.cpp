// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParameterToStateConnection.h"

#include <CrySerialization/Decorators/Range.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CParameterToStateConnection::Serialize(Serialization::IArchive& ar)
{
	float const value = m_value;

	if (m_itemType != EItemType::Category)
	{
		ar(Serialization::Range(m_value, 0.0f, 1.0f), "value", "Value");
	}
	else
	{
		ar(Serialization::Range(m_value, 0.0f, 1.0f), "volume", "Volume");
	}

	m_value = crymath::clamp(m_value, 0.0f, 1.0f);

	if (ar.isInput())
	{
		if (fabs(value - m_value) > g_precision)
		{
			SignalConnectionChanged();
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace ACE
