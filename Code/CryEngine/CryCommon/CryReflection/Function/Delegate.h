// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/Any.h>
#include <CryReflection/IFunctionDesc.h>
#include <CryReflection/Function/FunctionDelegate.h>

namespace Cry {
namespace Reflection {

class CDelegate
{
public:
	CDelegate(void* pObject, const IFunctionDesc& functionDesc)
		: m_pObject(pObject)
		, m_functionDesc(functionDesc)
	{

	}

	template<typename TYPE>
	CDelegate(TYPE* pObject, const IFunctionDesc& functionDesc)
		: m_pObject(static_cast<void*>(pObject))
		, m_functionDesc(functionDesc)
	{

	}

	// TODO: Maybe add sanity checks? (param count, ...)
	CAny Invoke() const
	{
		CFunctionExecutionContext context(m_pObject, m_functionDesc.GetParamCount());
		m_functionDesc.GetFunctionDelegate().Execute(context);

		return CAny(context.GetResult());
	}

	template<typename ... PARAM_TYPES>
	CAny Invoke(PARAM_TYPES ... params) const
	{
		CFunctionExecutionContext context(m_pObject, m_functionDesc.GetParamCount());
		size_t index = 0;
		for (const auto& param : { params ... })
		{
			context.SetParam(index++, param);
		}
		m_functionDesc.GetFunctionDelegate().Execute(context);

		return CAny(context.GetResult());
	}
	// ~TODO

	const IFunctionDesc& GetFunctionDesc() const
	{
		return m_functionDesc;
	}

private:
	void*                m_pObject;
	const IFunctionDesc& m_functionDesc;
};

} // ~Reflection namespace
} // ~Cry namespace
