// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <type_traits>

#include "Schematyc/Utils/IProperties.h"

namespace Schematyc
{
template<typename TYPE> class CProperties : public IProperties
{
	static_assert(std::is_copy_assignable<TYPE>::value, "Type must provide copy assignment operator!");

public:

	inline CProperties(const TYPE& value)
		: m_value(value)
	{}

	// IProperties

	virtual void* GetValue() override
	{
		return &m_value;
	}

	virtual const void* GetValue() const override
	{
		return &m_value;
	}

	virtual IPropertiesPtr Clone() const override
	{
		return std::make_shared<CProperties>(m_value);
	}

	virtual void Serialize(Serialization::IArchive& archive) override
	{
		m_value.Serialize(archive);
	}

	// IProperties

private:

	TYPE m_value;
};

namespace Properties
{
template<typename TYPE> inline std::shared_ptr<CProperties<TYPE>> MakeShared(const TYPE& value)
{
	return std::make_shared<CProperties<TYPE>>(value);
}
} // Properties
} // Schematyc
