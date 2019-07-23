// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParameterConnection.h"

namespace ACE
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Serialize(Serialization::IArchive& ar)
{
	float const minValue = m_minValue;
	float const maxValue = m_maxValue;
	float const mult = m_mult;
	float const shift = m_shift;

	if (m_type == EAssetType::Parameter)
	{
		ar(m_minValue, "min", "Min Value");
		m_minValue = std::min(m_minValue, m_maxValue);

		if (m_minValue == m_maxValue)
		{
			m_minValue = m_maxValue - 0.01f;
		}

		ar(m_maxValue, "max", "Max Value");
		m_maxValue = std::max(m_minValue, m_maxValue);

		if (m_minValue == m_maxValue)
		{
			m_maxValue = m_minValue + 0.01f;
		}
	}

	ar(m_mult, "mult", "Multiply");
	ar(m_shift, "shift", "Shift");

	if (ar.isInput())
	{
		if ((fabs(minValue - m_minValue) > g_precision) ||
		    (fabs(maxValue - m_maxValue) > g_precision) ||
		    (fabs(mult - m_mult) > g_precision) ||
		    (fabs(shift - m_shift) > g_precision))
		{
			SignalConnectionChanged();
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace ACE
