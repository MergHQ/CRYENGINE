// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "KeyConnection.h"

#include "Impl.h"

#include <CrySerialization/StringList.h>

namespace ACE
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
void CKeyConnection::Serialize(Serialization::IArchive& ar)
{
	string const event = m_event;

	Serialization::StringList allEvents;

	if (!(std::find(CImpl::s_programmerSoundEvents.begin(), CImpl::s_programmerSoundEvents.end(), m_event) != CImpl::s_programmerSoundEvents.end()))
	{
		allEvents.emplace_back(m_event);
	}

	for (auto const& str : CImpl::s_programmerSoundEvents)
	{
		allEvents.emplace_back(str);
	}

	Serialization::StringListValue const selectedEvent(allEvents, m_event);
	ar(selectedEvent, "event", "Programmer Sound Event");
	m_event = allEvents[selectedEvent.index()];
	ar.doc(m_event);

	if (ar.isInput())
	{
		if (event != m_event)
		{
			SignalConnectionChanged();
		}
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace ACE
