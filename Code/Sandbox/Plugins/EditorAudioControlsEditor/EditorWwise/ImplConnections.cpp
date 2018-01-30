// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ImplConnections.h"

namespace ACE
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
void CParameterConnection::Serialize(Serialization::IArchive& ar)
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
void CStateToParameterConnection::Serialize(Serialization::IArchive& ar)
{
	float const value = m_value;

	ar(m_value, "value", "Value");

	if (ar.isInput())
	{
		if (value != m_value)
		{
			SignalConnectionChanged();
		}
	}
}
} // namespace Wwise
} //namespace ACE
