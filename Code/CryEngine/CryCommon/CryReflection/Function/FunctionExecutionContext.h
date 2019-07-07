// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/Function/Function.h>
#include <CryReflection/Any.h>

namespace Cry {
namespace Reflection {

class CFunctionExecutionContext
{
private:
	template<typename RETURN_TYPE, bool IS_POINTER = std::is_pointer<RETURN_TYPE>::value>
	struct SetResultSelection
	{
		static void SetResult(CAny& member, const RETURN_TYPE& returnValue)
		{
			member.AssignValue<RETURN_TYPE>(returnValue);
		}
	};

	template<typename RETURN_TYPE>
	struct SetResultSelection<RETURN_TYPE*, true>
	{
		static void SetResult(CAny& member, RETURN_TYPE* returnValue)
		{
			member.AssignPointer<RETURN_TYPE>(returnValue);
		}
	};

public:
	CFunctionExecutionContext(void* pObject, size_t paramCount = 0)
		: m_pObject(pObject)
	{
		m_params.resize(paramCount);
	}

	template<typename OBJECT_TYPE>
	OBJECT_TYPE* GetObject() const
	{
		return static_cast<OBJECT_TYPE*>(m_pObject);
	}

	bool SetParam(size_t index, const CAny& any)
	{
		if (index < m_params.size())
		{
			m_params[index] = any;
			return true;
		}
		CRY_ASSERT(index < m_params.size(), "Function param 'index' (%i) out of range", index);
		return false;
	}

	size_t GetParamCount() const
	{
		return m_params.size();
	}

	template<typename TYPE>
	auto GetParam(size_t index)->typename std::remove_reference<TYPE>::type *
	{
		if (index < m_params.size())
		{
			return m_params[index].GetPointer<typename std::remove_reference<TYPE>::type>();
		}
		return nullptr;
	}

	template<typename RETURN_TYPE>
	void SetResult(const RETURN_TYPE& returnValue)
	{
		SetResultSelection<RETURN_TYPE>::SetResult(m_returnValue, returnValue);
	}

	template<typename RETURN_TYPE>
	const RETURN_TYPE* GetResult() const
	{
		return m_returnValue.GetPointer<RETURN_TYPE>();
	}

	const CAny& GetResult() const
	{
		return m_returnValue;
	}

private:
	void*             m_pObject;
	CAny              m_returnValue;
	std::vector<CAny> m_params;
};

} // ~Reflection namespace
} // ~Cry namespace
