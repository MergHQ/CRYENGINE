// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Do we need to worry about data alignment?
// #SchematycTODO : Implement move semantics?

#pragma once

#include <CryMath/Cry_Math.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Reflection/TypeDesc.h"
#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/Scratchpad.h"

namespace Schematyc
{

class CRuntimeParams
{
protected:

	enum class EParamType : uint16
	{
		Input,
		Output
	};

	struct SParam
	{
		inline SParam(uint32 _id, EParamType _type, uint16 _pos)
			: id(_id)
			, type(_type)
			, pos(_pos)
		{}

		uint32     id;
		EParamType type;
		uint32     pos;
	};

public:

	inline CRuntimeParams()
		: m_size(0)
		, m_capacity(0)
		, m_pParams(nullptr)
		, m_bInPlace(false)
	{}

	inline CRuntimeParams(const CRuntimeParams& rhs)
		: m_size(0)
		, m_capacity(0)
		, m_pParams(nullptr)
		, m_bInPlace(false)
	{
		Copy(rhs);
	}

	inline ~CRuntimeParams()
	{
		if (m_pParams && !m_bInPlace)
		{
			CryModuleFree(m_pParams);
		}
	}

	inline void Reserve(uint32 capacity)
	{
		if (capacity > m_capacity)
		{
			static const uint32 minCapacity = 64;
			static const uint32 growthFactor = 2;
			m_capacity = max(max(capacity, m_capacity * growthFactor), minCapacity);

			SParam* pParams = static_cast<SParam*>(CryModuleMalloc(m_capacity));
			std::copy(m_pParams, m_pParams + m_size, pParams);

			if (!m_bInPlace)
			{
				CryModuleFree(m_pParams);
			}

			m_pParams = pParams;
			m_bInPlace = false;
		}
	}

	inline bool SetInput(uint32 paramId, const CAnyConstRef& value)
	{
		return Set(paramId, EParamType::Input, value);
	}

	inline CAnyConstPtr GetInput(uint32 paramId) const
	{
		return Get(paramId, EParamType::Input);
	}

	inline bool GetInput(uint32 paramId, const CAnyRef& value) const
	{
		return Get(paramId, value, EParamType::Input);
	}

	inline bool SetOutput(uint32 paramId, const CAnyConstRef& value)
	{
		return Set(paramId, EParamType::Output, value);
	}

	inline CAnyConstPtr GetOutput(uint32 paramId) const
	{
		return Get(paramId, EParamType::Output);
	}

	inline void operator=(const CRuntimeParams& rhs)
	{
		Copy(rhs);
	}

protected:

	inline CRuntimeParams(const SInPlaceAllocationParams& params, const SInPlaceAllocationParams& scratchParams)
		: m_size(0)
		, m_capacity(params.capacity / sizeof(SParam))
		, m_pParams(static_cast<SParam*>(params.pStorage))
		, m_bInPlace(true)
		, m_scratchPad(scratchParams)
	{}

	inline void Copy(const CRuntimeParams& rhs)
	{
		Reserve(rhs.m_size);

		std::copy(rhs.m_pParams, rhs.m_pParams + rhs.m_size, m_pParams);

		m_size = rhs.m_size;
		m_scratchPad = rhs.m_scratchPad;
	}

private:

	inline bool Set(uint32 paramId, EParamType paramType, const CAnyConstRef& value)
	{
		uint32 pos = ParamIdToPos(paramId, paramType);
		if (pos != InvalidIdx)
		{
			return Any::CopyAssign(*m_scratchPad.Get(pos), value);
		}
		else if (paramId != InvalidId)
		{
			Reserve(m_size + 1);

			uint32 pos = m_scratchPad.Add(value);
			m_pParams[m_size++] = SParam(paramId, paramType, pos);

			return true;
		}
		return false;
	}

	inline CAnyConstPtr Get(uint32 paramId, EParamType paramType) const
	{
		const uint32 pos = ParamIdToPos(paramId, paramType);
		return pos != InvalidIdx ? m_scratchPad.Get(pos) : CAnyConstPtr();
	}

	inline bool Get(uint32 paramId, const CAnyRef& value, EParamType paramType) const
	{
		CAnyConstPtr pSrc = Get(paramId, paramType);
		return pSrc ? Any::CopyAssign(value, *pSrc) : false;
	}

	inline uint32 ParamIdToPos(uint32 paramId, EParamType paramType) const
	{
		for (const SParam* pParam = m_pParams, * pEnd = m_pParams + m_size; pParam != pEnd; ++pParam)
		{
			if ((pParam->id == paramId) && (pParam->type == paramType))
			{
				return pParam->pos;
			}
		}
		return InvalidIdx;
	}

private:

	uint32      m_size;
	uint32      m_capacity;
	SParam*     m_pParams;
	bool        m_bInPlace;
	CScratchpad m_scratchPad;
};

template<uint32 CAPACITY, uint32 SCRATCH_CAPACITY> class CInPlaceRuntimeParams : public CRuntimeParams
{
public:

	inline CInPlaceRuntimeParams()
		: CRuntimeParams(SInPlaceAllocationParams(CAPACITY, m_storage), SInPlaceAllocationParams(SCRATCH_CAPACITY, m_scratchStorage))
	{}

	inline CInPlaceRuntimeParams(const CRuntimeParams& rhs)
		: CRuntimeParams(SInPlaceAllocationParams(CAPACITY, m_storage), SInPlaceAllocationParams(SCRATCH_CAPACITY, m_scratchStorage))
	{
		CRuntimeParams::Copy(rhs);
	}

private:

	uint8 m_storage[CAPACITY];
	uint8 m_scratchStorage[SCRATCH_CAPACITY];
};

typedef CRuntimeParams                 HeapRuntimeParams;
typedef CInPlaceRuntimeParams<64, 128> StackRuntimeParams;

namespace RuntimeParams
{

inline bool FromInputClass(const CClassDesc& typeDesc, CRuntimeParams& output, const void* pInput)
{
	// #SchematycTODO : Implement some kind of type checking? Perhaps runtime params could store type guid?

	for (const CClassMemberDesc& memberDesc : typeDesc.GetMembers())
	{
		if (!output.SetInput(memberDesc.GetId(), CAnyConstRef(memberDesc.GetTypeDesc(), reinterpret_cast<const uint8*>(pInput) + memberDesc.GetOffset())))
		{
			return false;
		}
	}

	return true;
}

template<typename TYPE> inline bool FromInputClass(CRuntimeParams& output, const TYPE& input)
{
	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);
	static_assert(std::is_class<TYPE>::value, "Type must be class!");

	return FromInputClass(GetTypeDesc<TYPE>(), output, &input);
}

inline bool ToInputClass(const CClassDesc& typeDesc, void* pOutput, const CRuntimeParams& input)
{
	// #SchematycTODO : Implement some kind of type checking? Perhaps runtime params could store type guid?

	for (const CClassMemberDesc& member : typeDesc.GetMembers())
	{
		if (!input.GetInput(member.GetId(), CAnyRef(member.GetTypeDesc(), reinterpret_cast<uint8*>(pOutput) + member.GetOffset())))
		{
			return false;
		}
	}

	return true;
}

template<typename TYPE> inline bool ToInputClass(TYPE& output, const CRuntimeParams& input)
{
	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);
	static_assert(std::is_class<TYPE>::value, "Type must be class!");

	return ToInputClass(GetTypeDesc<TYPE>(), &output, input);
}

} // RuntimeParams
} // Schematyc
