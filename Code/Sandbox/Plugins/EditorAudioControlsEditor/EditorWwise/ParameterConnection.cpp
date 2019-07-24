// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParameterConnection.h"

#include <CrySerialization/Decorators/ActionButton.h>

namespace ACE
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
bool CParameterConnection::IsAdvanced() const
{
	return (m_mult != CryAudio::Impl::Wwise::g_defaultParamMultiplier) || (m_shift != CryAudio::Impl::Wwise::g_defaultParamShift);
}

//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Serialize(Serialization::IArchive& ar)
{
	float const mult = m_mult;
	float const shift = m_shift;

	ar(m_isAdvanced, "advanced", "Advanced");

	if (m_isAdvanced)
	{
		ar(m_mult, "mult", "Multiply");
		ar(m_shift, "shift", "Shift");

		ar(Serialization::ActionButton(functor(*this, &CParameterConnection::Reset)), "reset", "Reset");
	}

	if (ar.isInput())
	{
		if (fabs(mult - m_mult) > g_precision ||
		    fabs(shift - m_shift) > g_precision)
		{
			SignalConnectionChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Reset()
{
	bool const isAdvanced = IsAdvanced();

	m_mult = CryAudio::Impl::Wwise::g_defaultParamMultiplier;
	m_shift = CryAudio::Impl::Wwise::g_defaultParamShift;

	if (isAdvanced)
	{
		SignalConnectionChanged();
	}
}
} // namespace Wwise
} // namespace Impl
} // namespace ACE
