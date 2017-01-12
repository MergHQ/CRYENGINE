// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Schematyc
{
// Forward declare structures.


struct IGUIDRemapper
{
	virtual ~IGUIDRemapper() {}

	virtual bool  Empty() const = 0;
	virtual void  Bind(const SGUID& from, const SGUID& to) = 0;
	virtual SGUID Remap(const SGUID& from) const = 0;
};
} // Schematyc
