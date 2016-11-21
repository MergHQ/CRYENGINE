// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GeneratorBlueprint.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualGeneratorBlueprint
		//
		//===================================================================================

		CTextualGeneratorBlueprint::CTextualGeneratorBlueprint()
		{
		}

		void CTextualGeneratorBlueprint::SetGeneratorName(const char* generatorName)
		{
			m_generatorName = generatorName;
		}

		ITextualInputBlueprint& CTextualGeneratorBlueprint::GetInputRoot()
		{
			return m_rootInput;
		}

		const char* CTextualGeneratorBlueprint::GetGeneratorName() const
		{
			return m_generatorName.c_str();
		}

		const ITextualInputBlueprint& CTextualGeneratorBlueprint::GetInputRoot() const
		{
			return m_rootInput;
		}

		void CTextualGeneratorBlueprint::SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr)
		{
			m_pSyntaxErrorCollector = std::move(ptr);
		}

		datasource::ISyntaxErrorCollector* CTextualGeneratorBlueprint::GetSyntaxErrorCollector() const
		{
			return m_pSyntaxErrorCollector.get();
		}

		//===================================================================================
		//
		// CGeneratorBlueprint
		//
		//===================================================================================

		CGeneratorBlueprint::CGeneratorBlueprint()
			: m_pGeneratorFactory(nullptr)
		{}

		bool CGeneratorBlueprint::Resolve(const ITextualGeneratorBlueprint& source, const CQueryBlueprint& queryBlueprintForGlobalParamChecking)
		{
			const char* generatorName = source.GetGeneratorName();

			m_pGeneratorFactory = g_hubImpl->GetGeneratorFactoryDatabase().FindFactoryByName(generatorName);
			if (!m_pGeneratorFactory)
			{
				if (datasource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Unknown GeneratorFactory '%s'", generatorName);
				}
				return false;
			}

			CInputBlueprint inputRoot;
			const ITextualInputBlueprint& textualInputRoot = source.GetInputRoot();
			const client::IInputParameterRegistry& inputParamsReg = m_pGeneratorFactory->GetInputParameterRegistry();

			if (!inputRoot.Resolve(textualInputRoot, inputParamsReg, queryBlueprintForGlobalParamChecking, true))
			{
				return false;
			}

			ResolveInputs(inputRoot);

			return true;
		}

		const shared::CTypeInfo& CGeneratorBlueprint::GetTypeOfItemsToGenerate() const
		{
			assert(m_pGeneratorFactory);
			return m_pGeneratorFactory->GetTypeOfItemsToGenerate();
		}

		client::GeneratorUniquePtr CGeneratorBlueprint::InstantiateGenerator(const SQueryBlackboard& blackboard, shared::CUqsString& error) const
		{
			assert(m_pGeneratorFactory);

			//
			// create the input parameters (they will get filled by the function calls below)
			//

			client::ParamsHolderUniquePtr pParamsHolder = m_pGeneratorFactory->GetParamsHolderFactory().CreateParamsHolder();
			void* pParams = pParamsHolder->GetParams();

			//
			// fill all input parameters via function calls
			//

			CFunctionCallHierarchy functionCalls;

			if (!InstantiateFunctionCallHierarchy(functionCalls, blackboard, error))    // notice: the blackboard.pItemIterationContext is still a nullptr (we're not iterating on the items yet)
			{
				return nullptr;
			}

			bool bExceptionOccurredDuringFunctionCalls = false;
			const client::IFunction::SExecuteContext execContext(0, blackboard, error, bExceptionOccurredDuringFunctionCalls);  // currentItemIndex == 0: this is just a dummy, as we're not iterating on items

			functionCalls.ExecuteAll(execContext, pParams, m_pGeneratorFactory->GetInputParameterRegistry());

			if (bExceptionOccurredDuringFunctionCalls)
			{
				return nullptr;
			}

			//
			// instantiate the generator with the input params
			//

			client::GeneratorUniquePtr pGenerator = m_pGeneratorFactory->CreateGenerator(pParams);   // never returns NULL
			assert(pGenerator);
			return pGenerator;
		}

		void CGeneratorBlueprint::PrintToConsole(CLogger& logger) const
		{
			logger.Printf("Generator: %s [%s]", m_pGeneratorFactory->GetName(), m_pGeneratorFactory->GetTypeOfItemsToGenerate().name());
		}

	}
}
