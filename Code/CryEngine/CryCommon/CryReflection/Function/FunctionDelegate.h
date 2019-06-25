// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/Function/FunctionExecutionContext.h>

namespace Cry {
namespace Reflection {

class CFunctionDelegate
{
public:
	typedef void (* ProxyFunc)(const CFunctionPtr& functPtr, CFunctionExecutionContext& context);

	CFunctionDelegate()
		: m_proxyFunc(nullptr)
	{}

	CFunctionDelegate(CFunctionPtr functionPtr, ProxyFunc proxyFunc)
		: m_functionPtr(functionPtr)
		, m_proxyFunc(proxyFunc)
	{}

	template<typename FUNCTION_TYPE>
	CFunctionDelegate(FUNCTION_TYPE pFunction, ProxyFunc proxyFunc)
		: m_functionPtr(pFunction)
		, m_proxyFunc(proxyFunc)
	{}

	bool IsValid() const
	{
		return m_functionPtr.IsValid();
	}

	void Execute(CFunctionExecutionContext& context) const
	{
		if (m_proxyFunc && m_functionPtr.IsValid())
		{
			m_proxyFunc(m_functionPtr, context);
			return;
		}
		CRY_ASSERT(m_proxyFunc && m_functionPtr.IsValid(), "Pointer or proxy not set.");
	}

	void Execute_Fast(CFunctionExecutionContext& context) const
	{
		m_proxyFunc(m_functionPtr, context);
	}

private:
	ProxyFunc    m_proxyFunc;
	CFunctionPtr m_functionPtr;
};

} // ~Reflection namespace
} // ~Cry namespace
