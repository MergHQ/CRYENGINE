// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <type_traits>

namespace Cry {

template<typename VALUE_TYPE, bool IS_ENUM = std::is_enum<VALUE_TYPE>::value>
class CFlags
{
public:
	static const bool s_isValid = false;
};

template<typename VALUE_TYPE>
class CFlags<VALUE_TYPE, true>
{
public:
	typedef VALUE_TYPE                                      ValueType;
	typedef typename std::underlying_type<VALUE_TYPE>::type UnderlyingType;

	static const bool s_isValid = true;

public:
	CFlags() = default;

	CFlags(ValueType value)
		: m_value(static_cast<UnderlyingType>(value))
	{
	}

	CFlags(std::initializer_list<ValueType> values)
	{
		for (ValueType value : values)
		{
			m_value |= static_cast<UnderlyingType>(value);
		}
	}

	CFlags(const CFlags& rhs)
		: m_value(rhs.m_value)
	{
	}

	// TODO: Add type safety for enum class flags.
	void      SetFlag(ValueType flag)        { m_value |= static_cast<UnderlyingType>(flag); }
	void      SetFlags(CFlags flags)         { m_value |= flags.m_value; }

	bool      HasFlag(ValueType value) const { return ((m_value & static_cast<UnderlyingType>(value)) != 0); }
	bool      HasFlags(CFlags flags) const   { return (m_value == flags.m_value); }
	bool      HasAnyFlag(CFlags flags) const { return ((m_value & flags.m_value) != 0); }

	ValueType Value() const                  { return static_cast<ValueType>(m_value); }

	bool      IsEqual(ValueType value) const { return ((m_value & static_cast<UnderlyingType>(value)) == static_cast<UnderlyingType>(value)); }
	bool      IsEqual(CFlags flags) const    { return ((m_value & flags.m_value) == flags.m_value); }
	bool      IsEmpty() const                { return (m_value == 0); }

	void      RemoveFlag(ValueType value)    { m_value &= ~static_cast<UnderlyingType>(value); }
	void      RemoveFlags(CFlags flags)      { m_value &= ~flags.m_value; }

	void      Clear()                        { m_value = 0; }

	operator bool() const { return m_value != 0; }

	void    operator=(CFlags rhs)        { m_value = rhs.m_value; }

	bool    operator==(CFlags rhs) const { return m_value == rhs.m_value; }
	bool    operator!=(CFlags rhs) const { return m_value != rhs.m_value; }

	CFlags  operator|(CFlags rhs) const  { return CFlags(static_cast<ValueType>(m_value | rhs.m_value)); }
	CFlags  operator|=(CFlags rhs)       { m_value |= rhs.m_value; return *this; }

	CFlags  operator&(CFlags rhs) const  { return CFlags(static_cast<ValueType>(m_value & rhs.m_value)); }
	CFlags& operator&=(CFlags rhs)       { m_value &= rhs.m_value; return *this; }

	CFlags  operator^(CFlags rhs) const  { return CFlags(static_cast<ValueType>(m_value ^ rhs.m_value)); }
	CFlags  operator^=(CFlags rhs)       { m_value ^= rhs.m_value; return *this; }

	CFlags  operator~() const            { return CFlags(static_cast<ValueType>(~m_value)); }

private:
	UnderlyingType m_value = 0;
};
//~TODO

} // ~Cry namespace
