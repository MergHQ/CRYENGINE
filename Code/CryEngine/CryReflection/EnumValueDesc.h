// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/IEnumValueDesc.h>
#include <CryString/CryString.h>

namespace Cry {
namespace Reflection {

class CEnumValueDesc : public IEnumValueDesc
{
public:
	CEnumValueDesc();
	CEnumValueDesc(const char* szLabel, uint64 value, const char* szDescription);

	// IEnumValueDesc
	const char* GetLabel() const       { return m_label.c_str(); }
	const char* GetDescription() const { return m_description.c_str(); }
	uint64      GetValue() const       { return m_value; }
	// ~IEnumValueDesc

	bool operator==(const CEnumValueDesc& other) const
	{
		return m_label == other.m_label && m_value == other.m_value;
	}

	bool operator!=(const CEnumValueDesc& other) const
	{
		return m_label != other.m_label || m_value != other.m_value;
	}

private:
	string m_label;
	string m_description;
	uint64 m_value;
};

} // ~Reflection namespace
} // ~Cry namespace
