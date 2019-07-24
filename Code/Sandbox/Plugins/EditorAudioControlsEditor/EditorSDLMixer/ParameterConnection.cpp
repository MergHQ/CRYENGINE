// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParameterConnection.h"

#include <CrySerialization/Decorators/Range.h>
#include <CrySerialization/Decorators/ActionButton.h>

namespace ACE
{
namespace Impl
{
namespace SDLMixer
{
//////////////////////////////////////////////////////////////////////////
bool CParameterConnection::IsAdvanced() const
{
	return (m_mult != CryAudio::Impl::SDL_mixer::g_defaultParamMultiplier) || (m_shift != CryAudio::Impl::SDL_mixer::g_defaultParamShift);
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
		m_mult = std::max(0.0f, m_mult);

		ar(Serialization::Range(m_shift, -1.0f, 1.0f, 0.1f), "shift", "Shift");
		m_shift = crymath::clamp(m_shift, -1.0f, 1.0f);

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

	m_mult = CryAudio::Impl::SDL_mixer::g_defaultParamMultiplier;
	m_shift = CryAudio::Impl::SDL_mixer::g_defaultParamShift;

	if (isAdvanced)
	{
		SignalConnectionChanged();
	}
}
} // namespace SDLMixer
} // namespace Impl
} // namespace ACE
