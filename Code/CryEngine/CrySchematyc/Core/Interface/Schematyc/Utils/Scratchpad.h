// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Do we need to worry about data alignment?
// #SchematycTODO : Implement move semantics?

#pragma once

#include <type_traits>

#include <CryMath/Cry_Math.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Utils/Any.h"

namespace Schematyc
{
class CScratchpad
{
private:

	class CAnyValueImpl : public CAnyValue
	{
	public:

		inline CAnyValueImpl(const CCommonTypeInfo& typeInfo, const void* pValue)
			: CAnyValue(typeInfo, pValue)
		{}
	};

public:

	inline CScratchpad()
		: m_size(0)
		, m_capacity(0)
		, m_pData(nullptr)
		, m_bInPlace(false)
	{}

	inline CScratchpad(const SInPlaceAllocationParams& params)
		: m_size(0)
		, m_capacity(params.capacity)
		, m_pData(static_cast<uint8*>(params.pStorage))
		, m_bInPlace(true)
	{}

	inline CScratchpad(const CScratchpad& rhs)
		: m_size(0)
		, m_capacity(0)
		, m_pData(nullptr)
		, m_bInPlace(false)
	{
		Copy(rhs);
	}

	inline ~CScratchpad()
	{
		Clear();

		if (m_pData && !m_bInPlace)
		{
			CryModuleFree(m_pData);
		}
	}

	inline void Reserve(uint32 capacity)
	{
		if (capacity > m_capacity)
		{
			static const uint32 minCapacity = 256;
			static const uint32 growthFactor = 2;
			m_capacity = max(max(capacity, m_capacity * growthFactor), minCapacity);

			uint8* pData = static_cast<uint8*>(CryModuleMalloc(m_capacity));
			MoveData(pData, m_pData, m_size);

			if (!m_bInPlace)
			{
				CryModuleFree(m_pData);
			}

			m_pData = pData;
			m_bInPlace = false;
		}
	}

	inline uint32 Add(const CAnyConstRef& value)
	{
		const CCommonTypeInfo& typeInfo = value.GetTypeInfo();

		const uint32 size = m_size + sizeof(CAnyValueImpl) + typeInfo.GetSize();
		Reserve(size);

		const uint32 pos = m_size;
		m_size = size;

		new(m_pData + pos)CAnyValueImpl(typeInfo, value.GetValue());

		return pos;
	}

	inline CAnyPtr Get(uint32 pos)
	{
		return pos < m_size ? reinterpret_cast<CAnyValue*>(m_pData + pos) : CAnyPtr();
	}

	inline CAnyConstPtr Get(uint32 pos) const
	{
		return pos < m_size ? reinterpret_cast<const CAnyValue*>(m_pData + pos) : CAnyConstPtr();
	}

	inline void ShrinkToFit()
	{
		if (!m_bInPlace)
		{
			static const uint32 shrinkThreshold = 64;
			if ((m_capacity - m_size) >= shrinkThreshold)
			{
				uint8* pData = static_cast<uint8*>(CryModuleMalloc(m_capacity));
				MoveData(pData, m_pData, m_size);

				CryModuleFree(m_pData);

				m_capacity = m_size;
				m_pData = pData;
			}
		}
	}

	inline void Clear()
	{
		if (m_pData)
		{
			ClearData(m_pData, m_size);

			m_size = 0;
		}
	}

	inline void operator=(const CScratchpad& rhs)
	{
		Copy(rhs);
	}

protected:

	inline void Copy(const CScratchpad& rhs)
	{
		Clear();
		Reserve(rhs.m_size);
		CloneData(m_pData, rhs.m_pData, rhs.m_size);
		m_size = rhs.m_size;
	}

private:

	inline void CloneData(uint8* pDst, const uint8* pSrc, uint32 size) const
	{
		for (uint32 pos = 0; pos < size; )
		{
			const CAnyValueImpl* pSrcValue = reinterpret_cast<const CAnyValueImpl*>(pSrc + pos);
			const CCommonTypeInfo& typeInfo = pSrcValue->GetTypeInfo();
			new(pDst + pos)CAnyValueImpl(typeInfo, pSrcValue->GetValue());
			pos += sizeof(CAnyValueImpl) + typeInfo.GetSize();
		}
	}

	inline void ClearData(uint8* pData, uint32 size) const
	{
		for (uint32 pos = 0; pos < size; )
		{
			CAnyValueImpl* pValue = reinterpret_cast<CAnyValueImpl*>(pData + pos);
			const CCommonTypeInfo& typeInfo = pValue->GetTypeInfo();
			pos += sizeof(CAnyValueImpl) + typeInfo.GetSize();
			pValue->~CAnyValueImpl();
		}
	}

	inline void MoveData(uint8* pDst, uint8* pSrc, uint32 size) const
	{
		CloneData(pDst, pSrc, size);
		ClearData(pSrc, size);
	}

private:

	uint32 m_size;
	uint32 m_capacity;
	uint8* m_pData;
	bool   m_bInPlace;
};

template<uint32 CAPACITY> class CInPlaceScratchpad : public CScratchpad
{
public:

	inline CInPlaceScratchpad()
		: CScratchpad(SInPlaceAllocationParams(CAPACITY, m_storage))
	{}

	inline CInPlaceScratchpad(const CScratchpad& rhs)
		: CScratchpad(SInPlaceAllocationParams(CAPACITY, m_storage))
	{
		CScratchpad::Copy(rhs);
	}

private:

	uint8 m_storage[CAPACITY];
};

typedef CScratchpad             HeapScratchpad;
typedef CInPlaceScratchpad<256> StackScratchpad;
} // Schematyc
