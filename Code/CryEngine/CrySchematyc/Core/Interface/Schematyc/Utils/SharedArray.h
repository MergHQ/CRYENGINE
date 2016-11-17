// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/DynArray.h>
#include <CrySerialization/Forward.h>

namespace Schematyc
{
template<typename TYPE> class CSharedArray
{
	friend bool Serialize<TYPE>(Serialization::IArchive& archive, CSharedArray<TYPE>& value, const char* szName, const char* szLabel);

private:

	typedef DynArray<TYPE, uint32> Array;
	typedef std::shared_ptr<Array> ArrayPtr;

public:

	inline CSharedArray() {}

	inline CSharedArray(const CSharedArray& rhs)
		: m_pArray(rhs.m_pArray)
	{}

	inline uint32 GetSize() const
	{
		return m_pArray ? m_pArray->size() : 0;
	}

	inline void PushBack(const TYPE& value)
	{
		MakeUnique();
		m_pArray->push_back(value);
	}

	inline void PopBack()
	{
		CRY_ASSERT(m_pArray);
		MakeUnique();
		m_pArray->pop_back();
	}

	inline void SetElement(uint32 idx, const TYPE& value) const
	{
		CRY_ASSERT(m_pArray);
		MakeUnique();
		(*m_pArray)[idx] = value;
	}

	inline const TYPE& GetElement(uint32 idx) const
	{
		CRY_ASSERT(m_pArray);
		return (*m_pArray)[idx];
	}

	inline bool operator==(const CSharedArray& rhs) const
	{
		if (m_pArray != rhs.m_pArray)
		{
			return false;
		}

		const uint32 size = GetSize();
		if (size != rhs.GetSize())
		{
			return false;
		}

		for (uint32 pos = 0; pos < size; ++pos)
		{
			if ((*m_pArray)[pos] != (*rhs.m_pArray)[pos])
			{
				return false;
			}
		}

		return true;
	}

	inline bool operator!=(const CSharedArray& rhs) const
	{
		return !(*this == rhs);
	}

private:

	inline void MakeUnique()
	{
		if (!m_pArray)
		{
			m_pArray = std::make_shared<Array>();
		}
		else if (!m_pArray.unique())
		{
			m_pArray = std::make_shared<Array>(*m_pArray);
		}
	}

private:

	ArrayPtr m_pArray;
};

template<typename TYPE> inline bool Serialize(Serialization::IArchive& archive, CSharedArray<TYPE>& value, const char* szName, const char* szLabel)
{
	value.MakeUnique();
	archive(*value.m_pArray, szName, szLabel);
	return true;
}
} // Schematyc
