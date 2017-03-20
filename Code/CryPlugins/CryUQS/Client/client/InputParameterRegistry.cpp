// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "InputParameterRegistry.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Client
	{
		namespace Internal
		{

			CInputParameterRegistry::SStoredParameterInfo::SStoredParameterInfo(const char* _name, const Shared::CTypeInfo& _type, size_t _offset)
				: name(_name)
				, type(_type)
				, offset(_offset)
			{
				// nothing
			}

			void CInputParameterRegistry::RegisterParameterType(const char* paramName, const Shared::CTypeInfo& typeInfo, size_t offset)
			{
				// prevent duplicates
				assert(std::find_if(m_parametersInOrder.cbegin(), m_parametersInOrder.cend(), [paramName](const SStoredParameterInfo& p) { return p.name == paramName; }) == m_parametersInOrder.cend());

				SStoredParameterInfo pi(paramName, typeInfo, offset);
				m_parametersInOrder.push_back(pi);
			}

			size_t CInputParameterRegistry::GetParameterCount() const
			{
				return m_parametersInOrder.size();
			}

			IInputParameterRegistry::SParameterInfo CInputParameterRegistry::GetParameter(size_t index) const
			{
				assert(index < m_parametersInOrder.size());
				const SStoredParameterInfo& pi = m_parametersInOrder[index];
				return SParameterInfo(pi.name.c_str(), pi.type, pi.offset);
			}

		}
	}
}
