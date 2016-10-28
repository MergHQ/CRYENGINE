// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

struct SItemTypeName
{
	SItemTypeName();
	SItemTypeName(const char* szTypeName);
	SItemTypeName(const SItemTypeName& other);
	SItemTypeName(SItemTypeName&& other);
	SItemTypeName& operator=(const SItemTypeName& other);
	SItemTypeName& operator=(SItemTypeName&& other);

	void        Serialize(Serialization::IArchive& archive);
	const char* c_str() const;

	bool        Empty() const;

	bool        operator==(const SItemTypeName& other) const;
	bool        operator!=(const SItemTypeName& other) const { return !(*this == other); }

	string typeName;
};
