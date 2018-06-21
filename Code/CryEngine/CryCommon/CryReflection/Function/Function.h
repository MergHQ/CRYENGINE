// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

class CFunctionPtr
{
public:
	CFunctionPtr()
	{
		static_assert(sizeof(m_pointer) == (sizeof(uint64) * 2), "Please adapt code to buffer size.");
		uint64* ptr = reinterpret_cast<uint64*>(m_pointer);
		ptr[0] = ptr[1] = 0;
	}

	CFunctionPtr(const CFunctionPtr& functionPtr)
	{
		memcpy(m_pointer, &functionPtr, sizeof(m_pointer));
	}

	template<typename FUNCTION_TYPE>
	CFunctionPtr(FUNCTION_TYPE pFunction)
	{
		static_assert(sizeof(FUNCTION_TYPE) <= sizeof(m_pointer), "Buffer for function pointer too small.");
		memcpy(m_pointer, &pFunction, sizeof(FUNCTION_TYPE));
	}

	template<typename FUNCTION_TYPE>
	FUNCTION_TYPE Get() const
	{
		FUNCTION_TYPE pFunction;
		memcpy(&pFunction, m_pointer, sizeof(FUNCTION_TYPE));
		return pFunction;
	}

	bool IsValid() const
	{
		static_assert(sizeof(m_pointer) == (sizeof(uint64) * 2), "Please adapt code to buffer size.");
		const uint64* ptr = reinterpret_cast<const uint64*>(m_pointer);
		return ptr[0] || ptr[1];
	}

private:
	unsigned char m_pointer[sizeof(uint64) * 2];
};

} // ~Reflection namespace
} // ~Cry namespace
