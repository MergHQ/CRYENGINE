// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CFunctionCallHierarchy
		//
		// - helper class for commonly used code that needs to make a list of function calls and store the return values in a CVariantArray
		// - once this function hierarchy is built it can be re-used as often as desired
		//
		//===================================================================================

		class CFunctionCallHierarchy
		{
		public:
			explicit                                      CFunctionCallHierarchy() {}
			bool                                          AddAndInstantiateFunctionBlueprint(const CFunctionBlueprint& functionBlueprintToInstantiate, const SQueryBlackboard& blackboard, shared::CUqsString& error);
			void                                          ExecuteAll(const client::IFunction::SExecuteContext& executeContext, void* pParamsToWriteTheReturnValuesTo, const client::IInputParameterRegistry& registryToLookupParamsOffsets) const;

		private:
			                                              UQS_NON_COPYABLE(CFunctionCallHierarchy);

		private:
			std::vector<client::FunctionUniquePtr>        m_functionsToCall;
		};

	}
}
