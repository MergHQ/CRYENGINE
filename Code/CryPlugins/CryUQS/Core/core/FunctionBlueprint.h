// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CFunctionBlueprint
		//
		//===================================================================================

		class CFunctionBlueprint : public CBlueprintWithInputs
		{
		public:
			explicit                         CFunctionBlueprint(client::IFunctionFactory& functionFactory, const CLeafFunctionReturnValue& leafFunctionReturnValue, bool bAddReturnValueToDebugRenderWorldUponExecution);

			client::FunctionUniquePtr        InstantiateCallHierarchy(const SQueryBlackboard& blackboard, shared::CUqsString& error) const;        // returns nullptr if runtime-param type validation failed or insufficient params were provided
			client::IFunctionFactory&        GetFactory() const;

		private:
			                                 UQS_NON_COPYABLE(CFunctionBlueprint);

		private:
			client::IFunctionFactory&        m_functionFactory;                 // creates the "live" function at runtime
			CLeafFunctionReturnValue         m_returnValueInCaseOfLeafFunction; // this is the return value in case of a leaf-function (they don't have input)
			bool                             m_bAddReturnValueToDebugRenderWorldUponExecution;
		};

	}
}
