// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "BlueprintWithInputs.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		CBlueprintWithInputs::~CBlueprintWithInputs()
		{
			for(CFunctionBlueprint* pFuncBP : m_resolvedInputs)
			{
				delete pFuncBP;
			}
		}

		bool CBlueprintWithInputs::InstantiateFunctionCallHierarchy(CFunctionCallHierarchy& out, const SQueryBlackboard& blackboard, shared::CUqsString& error) const
		{
			for (size_t i = 0; i < m_resolvedInputs.size(); ++i)
			{
				const CFunctionBlueprint* pFuncBP = m_resolvedInputs[i];
				assert(pFuncBP);
				if (!out.AddAndInstantiateFunctionBlueprint(*pFuncBP, blackboard, error))
				{
					return false;
				}
			}
			return true;
		}

		void CBlueprintWithInputs::ResolveInputs(const CInputBlueprint& rootOfInputs)
		{
			for(size_t i = 0; i < rootOfInputs.GetChildCount(); ++i)
			{
				const CInputBlueprint& input = rootOfInputs.GetChild(i);
				client::IFunctionFactory* pFunctionFactory = input.GetFunctionFactory();
				assert(pFunctionFactory);
				const char* returnValueForLeafFunction = input.GetFunctionReturnValueLiteral();
				bool bAddReturnValueToDebugRenderWorldUponExecution = input.GetAddReturnValueToDebugRenderWorldUponExecution();
				CFunctionBlueprint* bp = new CFunctionBlueprint(*pFunctionFactory, returnValueForLeafFunction, bAddReturnValueToDebugRenderWorldUponExecution);
				m_resolvedInputs.push_back(bp);
				bp->ResolveInputs(input);
			}
		}

	}
}
