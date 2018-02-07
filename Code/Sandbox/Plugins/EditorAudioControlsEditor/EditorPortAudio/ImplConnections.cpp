// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ImplConnections.h"

#include <CrySerialization/Enum.h>

namespace ACE
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
void CConnection::Serialize(Serialization::IArchive& ar)
{
	EConnectionType const type = m_type;
	uint32 const loopCount = m_loopCount;
	bool const isInfiniteLoop = m_isInfiniteLoop;

	ar(m_type, "action", "Action");

	if (m_type == EConnectionType::Start)
	{
		ar(m_isInfiniteLoop, "infinite_loop", "Infinite Loop");

		if (!m_isInfiniteLoop)
		{
			ar(m_loopCount, "loop_count", "Loop Count");
			m_loopCount = std::max<uint32>(1, m_loopCount);
		}
	}

	if (ar.isInput())
	{
		if (type != m_type ||
		    loopCount != m_loopCount ||
		    isInfiniteLoop != m_isInfiniteLoop)
		{
			SignalConnectionChanged();
		}
	}
}

SERIALIZATION_ENUM_BEGIN(EConnectionType, "Event Type")
SERIALIZATION_ENUM(EConnectionType::Start, "start", "Start")
SERIALIZATION_ENUM(EConnectionType::Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()
} // namespace PortAudio
} //namespace ACE
