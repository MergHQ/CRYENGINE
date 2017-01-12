// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/Assert.h"
#include "Schematyc/Utils/Delegate.h"

namespace Schematyc
{
namespace EnvUtils
{

// #SchematycTODO : Do we still need IEnvElement::VisitChildren?
inline void VisitChildren(const IEnvElement& envElement, const CDelegate<void(const IEnvElement&)>& visitor)
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const IEnvElement* pEnvChildElement = envElement.GetFirstChild(); pEnvChildElement; pEnvChildElement = pEnvChildElement->GetNextSibling())
		{
			visitor(*pEnvChildElement);
		}
	}
}

template<typename TYPE> inline void VisitChildren(const IEnvElement& envElement, const CDelegate<void(const TYPE&)>& visitor)
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const IEnvElement* pEnvChildElement = envElement.GetFirstChild(); pEnvChildElement; pEnvChildElement = pEnvChildElement->GetNextSibling())
		{
			const TYPE* pEnvChildElementImpl = DynamicCast<TYPE>(pEnvChildElement);
			if (pEnvChildElementImpl)
			{
				visitor(*pEnvChildElementImpl);
			}
		}
	}
}

} // EnvUtils
} // Schematyc
