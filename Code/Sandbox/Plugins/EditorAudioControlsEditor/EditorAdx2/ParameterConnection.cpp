// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParameterConnection.h"

#include <CrySerialization/Decorators/ActionButton.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
bool CParameterConnection::IsAdvanced() const
{
	return (m_minValue != CryAudio::Impl::Adx2::g_defaultParamMinValue) ||
	       (m_maxValue != CryAudio::Impl::Adx2::g_defaultParamMaxValue) ||
	       (m_mult != CryAudio::Impl::Adx2::g_defaultParamMultiplier) ||
	       (m_shift != CryAudio::Impl::Adx2::g_defaultParamShift);
}

//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Serialize(Serialization::IArchive& ar)
{
	float const minValue = m_minValue;
	float const maxValue = m_maxValue;
	float const mult = m_mult;
	float const shift = m_shift;

	ar(m_isAdvanced, "advanced", "Advanced");

	if (m_isAdvanced)
	{
		ar(m_mult, "mult", "Multiply");
		ar(m_shift, "shift", "Shift");

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

		ar(Serialization::ActionButton(functor(*this, &CParameterConnection::Reset)), "reset", "Reset");
	}

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

//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Reset()
{
	bool const isAdvanced = IsAdvanced();

	m_minValue = CryAudio::Impl::Adx2::g_defaultParamMinValue;
	m_maxValue = CryAudio::Impl::Adx2::g_defaultParamMaxValue;
	m_mult = CryAudio::Impl::Adx2::g_defaultParamMultiplier;
	m_shift = CryAudio::Impl::Adx2::g_defaultParamShift;

	if (isAdvanced)
	{
		SignalConnectionChanged();
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace ACE
