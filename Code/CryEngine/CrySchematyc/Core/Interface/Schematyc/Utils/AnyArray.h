// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Separate into two classes: CAnyArray and CAnyArrayPtr!!!
// #SchematycTODO : Implement comparison operators?
// #SchematycTODO : Do we need to worry about data alignment?
// #SchematycTODO : Implement move semantics?

#pragma once

#include <CrySerialization/DynArray.h>
#include <CrySerialization/Forward.h>

#include "Schematyc/Reflection/Reflection.h"
#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/Assert.h"

namespace Schematyc
{
class CAnyArray
{
public:

	inline CAnyArray(const CCommonTypeInfo& typeInfo)
		: m_typeInfo(typeInfo)
		, m_capacity(0)
		, m_pBegin(nullptr)
		, m_pEnd(nullptr)
	{}

	inline CAnyArray(const CAnyArray& rhs)
		: m_typeInfo(rhs.m_typeInfo)
		, m_capacity(0)
		, m_pBegin(nullptr)
		, m_pEnd(nullptr)
	{
		Reserve(rhs.m_capacity);
		CopyConstruct(rhs.m_pBegin, rhs.m_pEnd, m_pBegin);

		m_pEnd = m_pBegin + (rhs.m_pEnd - rhs.m_pBegin);
	}

	inline ~CAnyArray()
	{
		if (m_pBegin)
		{
			Destruct(m_pBegin, m_pEnd);
			CryModuleFree(m_pBegin);
		}
	}

	inline uint32 GetSize() const
	{
		return static_cast<uint32>(m_pEnd - m_pBegin) / m_typeInfo.GetSize();
	}

	inline uint32 GetCapacity() const
	{
		return m_capacity;
	}

	inline void Reserve(uint32 capacity)
	{
		if (capacity > m_capacity)
		{
			const uint32 minCapacity = 16;
			const uint32 growthFactor = 2;
			capacity = max(capacity, max(minCapacity, capacity * growthFactor));

			uint8* pBegin = static_cast<uint8*>(CryModuleRealloc(m_pBegin, capacity));
			if (pBegin != m_pBegin)
			{
				Move(m_pBegin, m_pEnd, pBegin);

				m_pEnd = pBegin + (m_pEnd - m_pBegin);
				m_pBegin = pBegin;
			}

			m_capacity = capacity;
		}
	}

	inline void PushBack(const CAnyConstRef& value)
	{
		Reserve(GetSize() + 1);

		SCommonTypeInfoMethods::CopyConstruct copyConstruct = m_typeInfo.GetMethods().copyConstruct;
		SCHEMATYC_CORE_ASSERT(copyConstruct);

		(*copyConstruct)(m_pEnd, value.GetValue());

		m_pEnd += m_typeInfo.GetSize();
	}

	inline void PopBack()
	{
		m_pEnd -= m_typeInfo.GetSize();

		SCommonTypeInfoMethods::Destruct destruct = m_typeInfo.GetMethods().destruct;
		SCHEMATYC_CORE_ASSERT(destruct);

		(*destruct)(m_pEnd);
	}

	inline void RemoveByIdx(uint32 idx)
	{
		// #SchematycTODO : Validate idx?

		if (idx == (GetSize() - 1))
		{
			PopBack();
		}
		else
		{
			// #SchematycTODO : Remove elements rather than destroying and re-constructing?

			SCommonTypeInfoMethods::Destruct destruct = m_typeInfo.GetMethods().destruct;
			SCHEMATYC_CORE_ASSERT(destruct);

			SCommonTypeInfoMethods::CopyConstruct copyConstruct = m_typeInfo.GetMethods().copyConstruct;
			SCHEMATYC_CORE_ASSERT(copyConstruct);

			const uint32 stride = m_typeInfo.GetSize();
			for (uint8* pPos = m_pBegin + idx, * pNext = pPos + stride; pNext < m_pEnd; pPos = pNext, pNext += stride)
			{
				(*destruct)(pPos);
				(*copyConstruct)(pPos, pNext);
			}
			m_pEnd -= stride;
		}
	}

	inline void RemoveByValue(const CAnyConstRef& value)
	{
		SCommonTypeInfoMethods::Equals equals = m_typeInfo.GetMethods().equals;
		SCHEMATYC_CORE_ASSERT(equals);

		for (uint32 idx = 0, size = GetSize(); idx < size; )
		{
			if ((*equals)((*this)[idx].GetValue(), value.GetValue()))
			{
				RemoveByIdx(idx);
				--size;
			}
			else
			{
				++idx;
			}
		}
	}

	inline void Clear()
	{
		Destruct(m_pBegin, m_pEnd);

		m_pEnd = m_pBegin;
	}

	inline void ToString(IString& output) const
	{
		output.assign("{ ");

		SCommonTypeInfoMethods::ToString toString = m_typeInfo.GetMethods().toString;
		if (toString)
		{
			for (uint32 idx = 0, size = GetSize(); idx < size; ++idx)
			{
				CStackString temp;
				(*toString)(temp, (*this)[idx].GetValue());
				output.append(temp.c_str());
				output.append(", ");
			}
			output.TrimRight(", ");
		}

		output.append(" }");
	}

	inline CAnyRef operator[](uint32 idx)
	{
		return CAnyRef(m_typeInfo, m_pBegin + (idx * m_typeInfo.GetSize()));
	}

	inline CAnyConstRef operator[](uint32 idx) const
	{
		return CAnyRef(m_typeInfo, m_pBegin + (idx * m_typeInfo.GetSize()));
	}

	//inline CAnyArray& operator = (const CAnyArray& rhs);
	//inline bool operator == (const CAnyArray& rhs) const
	//inline bool operator != (const CAnyArray& rhs) const

	static inline SGUID ReflectSchematycType(CTypeInfo<CAnyArray>& typeInfo)
	{
		typeInfo.SetToStringMethod<& CAnyArray::ToString>();
		return "f6af4221-8344-49e9-9ef8-5f7e8144aa57"_schematyc_guid;
	}

private:

	void CopyConstruct(const uint8* pSrc, const uint8* pEnd, uint8* pDst) const
	{
		SCommonTypeInfoMethods::CopyConstruct copyConstruct = m_typeInfo.GetMethods().copyConstruct;
		SCHEMATYC_CORE_ASSERT(copyConstruct);

		for (; pSrc < pEnd; ++pSrc, ++pDst)
		{
			(*copyConstruct)(pDst, pSrc);
		}
	}

	void Move(uint8* pSrc, uint8* pEnd, uint8* pDst) const
	{
		SCommonTypeInfoMethods::CopyConstruct copyConstruct = m_typeInfo.GetMethods().copyConstruct;
		SCHEMATYC_CORE_ASSERT(copyConstruct);

		SCommonTypeInfoMethods::Destruct destruct = m_typeInfo.GetMethods().destruct;
		SCHEMATYC_CORE_ASSERT(destruct);

		for (; pSrc < pEnd; ++pSrc, ++pDst)
		{
			(*copyConstruct)(pDst, pSrc);
			(*destruct)(pSrc);
		}
	}

	void Destruct(uint8* pPos, uint8* pEnd) const
	{
		SCommonTypeInfoMethods::Destruct destruct = m_typeInfo.GetMethods().destruct;
		SCHEMATYC_CORE_ASSERT(destruct);

		for (; pPos < pEnd; ++pPos)
		{
			(*destruct)(pPos);
		}
	}

private:

	const CCommonTypeInfo& m_typeInfo;   // #SchematycTODO : Rather than storing type info could we just store size, copyConstruct and destruct?
	uint32                 m_capacity;
	uint8*                 m_pBegin;
	uint8*                 m_pEnd;   // #SchematycTODO : Store size explicitly as uint32?
};

class CAnyArrayPtr // #SchematycTODO : Create more generic pointer class for storing all types of value on scratchpad? Or just use CAnyPtr?
{
public:

	inline CAnyArrayPtr(CAnyArray* pArray = nullptr)
		: m_pArray(pArray)
	{}

	inline CAnyArrayPtr(const CAnyArrayPtr& rhs)
		: m_pArray(rhs.m_pArray)
	{}

	explicit inline operator bool() const
	{
		return m_pArray != nullptr;
	}

	inline CAnyArray* operator->()
	{
		return m_pArray;
	}

	inline const CAnyArray* operator->() const
	{
		return m_pArray;
	}

	inline CAnyArray& operator*()
	{
		SCHEMATYC_CORE_ASSERT(m_pArray);
		return *m_pArray;
	}

	inline const CAnyArray& operator*() const
	{
		SCHEMATYC_CORE_ASSERT(m_pArray);
		return *m_pArray;
	}

	CAnyArrayPtr& operator=(const CAnyArrayPtr& rhs)
	{
		m_pArray = rhs.m_pArray;
		return *this;
	}

	static inline SGUID ReflectSchematycType(CTypeInfo<CAnyArrayPtr>& typeInfo)
	{
		return "9500b20f-4264-4a09-a7ec-6c8136113369"_schematyc_guid;
	}

private:

	CAnyArray* m_pArray;
};
}   // Schematyc
