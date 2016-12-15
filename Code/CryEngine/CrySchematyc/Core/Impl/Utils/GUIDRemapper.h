// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/IGUIDRemapper.h"

namespace Schematyc
{
class CGUIDRemapper : public IGUIDRemapper
{
private:

	typedef std::unordered_map<SGUID, SGUID> GUIDs;

public:

	// IGUIDRemapper
	virtual bool  Empty() const override;
	virtual void  Bind(const SGUID& from, const SGUID& to) override;
	virtual SGUID Remap(const SGUID& from) const override;
	// ~IGUIDRemapper

private:

	GUIDs m_guids;
};
} // Schematyc
