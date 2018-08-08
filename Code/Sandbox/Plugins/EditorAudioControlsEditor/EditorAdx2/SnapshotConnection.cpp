// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SnapshotConnection.h"

namespace ACE
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CSnapshotConnection::Serialize(Serialization::IArchive& ar)
{
	int const changeoverTime = m_changeoverTime;

	ar(m_changeoverTime, "time", "Changeover Time (ms)");
	m_changeoverTime = std::max(0, m_changeoverTime);

	if (ar.isInput())
	{
		if (changeoverTime != m_changeoverTime)
		{
			SignalConnectionChanged();
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace ACE
