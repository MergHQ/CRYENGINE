// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GUIDRemapper.h"

namespace Schematyc
{
bool CGUIDRemapper::Empty() const
{
	return m_guids.empty();
}

void CGUIDRemapper::Bind(const SGUID& from, const SGUID& to)
{
	m_guids.insert(GUIDs::value_type(from, to));
}

SGUID CGUIDRemapper::Remap(const SGUID& from) const
{
	GUIDs::const_iterator itGUID = m_guids.find(from);
	return itGUID != m_guids.end() ? itGUID->second : from;
}
} // Schematyc
