// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

namespace Cry {

template<typename VALUE_TYPE, VALUE_TYPE INVALID_INDEX = std::numeric_limits<VALUE_TYPE>::max()>
struct Index
{
	typedef VALUE_TYPE ValueType;

	static const ValueType Invalid = INVALID_INDEX;
	static const ValueType MaxValue = INVALID_INDEX > 0 ? INVALID_INDEX - 1 : std::numeric_limits<ValueType>::max();
	static const ValueType MinValue = 0;

	static_assert(INVALID_INDEX != MaxValue && INVALID_INDEX != MinValue, "INVALID_INDEX must be greater than zero.");

	Index()
		: m_value(Invalid)
	{}

	Index(ValueType index)
		: m_value(index)
	{}

	explicit Index(size_t index)
	{
		if (index < std::numeric_limits<ValueType>::max())
		{
			m_value = static_cast<ValueType>(index);
		}
		else
		{
			CRY_ASSERT(index < std::numeric_limits<ValueType>::max(), "Index out of range.");
			m_value = Invalid;
		}
	}

	Index(const Index& other)
		: m_value(other.m_value)
	{}

	bool IsValid() const
	{
		return (m_value >= MinValue && m_value <= MaxValue);
	}

	static Index Empty()
	{
		return Index(Invalid);
	}

	ValueType ToValueType() const
	{
		return m_value;
	}

	template<typename CAST_TYPE>
	CAST_TYPE To() const
	{
		static_assert(sizeof(CAST_TYPE) >= sizeof(m_value), "Invalid cast to smaller type.");
		return static_cast<CAST_TYPE>(m_value);
	}

	operator ValueType() const
	{
		return m_value;
	}

	Index& operator++()
	{
		++m_value;
		return *this;
	}

	Index& operator--()
	{
		--m_value;
		return *this;
	}

	Index operator++(int)
	{
		Index tmp(m_value);
		++m_value;
		return tmp;
	}

	Index operator--(int)
	{
		Index tmp(m_value);
		--m_value;
		return tmp;
	}

	Index& operator+=(Index index)
	{
		m_value += index.m_value;
		return *this;
	}

	Index& operator-=(Index index)
	{
		m_value -= index.m_value;
		return *this;
	}

	Index& operator+=(ValueType value)
	{
		m_value += value;
		return *this;
	}

	Index& operator-=(ValueType value)
	{
		m_value -= value;
		return *this;
	}

	Index operator+(Index index) const
	{
		ValueType value = static_cast<ValueType>(m_value + index.m_value);
		return Index(value);
	}

	Index operator+(ValueType value) const
	{
		return Index(size_t(m_value + value));
	}

	Index operator-(Index index) const
	{
		ValueType value = static_cast<ValueType>(m_value - index.m_value);
		return Index(value);
	}

	Index operator-(ValueType value) const
	{
		return Index(size_t(m_value - value));
	}

private:
	bool Serialize(Serialization::IArchive& archive)
	{
		CRY_ASSERT(0, "Index is only valid during runtime and should not be serialized (to file).");
		return false;
	}

private:
	ValueType m_value;
};

} // ~Cry namespace
