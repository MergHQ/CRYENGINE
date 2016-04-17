// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>
#include "ACETypes.h"
#include <CrySerialization/IArchive.h>
#include <CrySerialization/STL.h>

namespace ACE
{
class IAudioSystemItem;
class IAudioConnection
{
public:
	IAudioConnection(CID nID)
		: m_nID(nID)
		, m_sGroup("default")
	{}

	virtual ~IAudioConnection() {}

	CID           GetID() const                          { return m_nID; }

	const string& GetGroup() const                       { return m_sGroup; }
	void          SetGroup(const string& group)          { m_sGroup = group; }

	virtual bool  HasProperties()                        { return false; }

	virtual void  Serialize(Serialization::IArchive& ar) {};

private:
	CID    m_nID;
	string m_sGroup;
};
}
