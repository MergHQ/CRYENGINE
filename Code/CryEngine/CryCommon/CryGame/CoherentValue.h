// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryScriptSystem/IScriptSystem.h> // <> required for Interfuscator
#include <CryNetwork/ISerialize.h>

// A value which can tell whether it's changed
template<typename T>
class CCoherentValue
{
public:
	ILINE CCoherentValue()
		: m_dirty(false)
	{
	}

	ILINE CCoherentValue(const T& val)
		: m_val(val)
		, m_dirty(true)
	{
	}

	ILINE CCoherentValue(const CCoherentValue& other)
	{
		if (m_val != other.m_val)
		{
			m_dirty = true;
		}
		m_val = other.m_val;
	}

	ILINE CCoherentValue& operator=(const CCoherentValue& rhs)
	{
		if (this != &rhs)
		{
			if (m_val != rhs.m_val)
			{
				m_dirty = true;
			}
			m_val = rhs.m_val;
		}
		return *this;
	}

	ILINE CCoherentValue operator+=(const CCoherentValue& rhs)
	{
		m_dirty = true;
		m_val += rhs.m_val;

		return *this;
	}

	ILINE bool IsDirty() const
	{
		return m_dirty;
	}

	ILINE void Clear()
	{
		m_dirty = false;
	}

	ILINE const T& Value() const
	{
		return m_val;
	}

	ILINE void SetDirtyValue(CScriptSetGetChain& chain, const char* name)
	{
		if (IsDirty())
		{
			chain.SetValue(name, m_val);
			Clear();
		}
	}

	ILINE void Serialize(TSerialize ser, const char* name)
	{
		ser.Value(name, m_val);
		if (ser.IsReading())
		{
			m_dirty = true;
		}
	}

	ILINE operator T() const
	{
		return m_val;
	}
private:
	T    m_val;
	bool m_dirty;
};
