// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GeneratorBlueprint.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CTextualGeneratorBlueprint
		//
		//===================================================================================

		CTextualGeneratorBlueprint::CTextualGeneratorBlueprint()
		{
		}

		void CTextualGeneratorBlueprint::SetGeneratorName(const char* szGeneratorName)
		{
			m_generatorName = szGeneratorName;
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

		void CTextualGeneratorBlueprint::SetSyntaxErrorCollector(DataSource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector)
		{
			m_pSyntaxErrorCollector = std::move(pSyntaxErrorCollector);
		}

		DataSource::ISyntaxErrorCollector* CTextualGeneratorBlueprint::GetSyntaxErrorCollector() const
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
			const char* szGeneratorName = source.GetGeneratorName();

			m_pGeneratorFactory = g_pHub->GetGeneratorFactoryDatabase().FindFactoryByName(szGeneratorName);
			if (!m_pGeneratorFactory)
			{
				if (DataSource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Unknown GeneratorFactory '%s'", szGeneratorName);
				}
				return false;
			}

			CInputBlueprint inputRoot;
			const ITextualInputBlueprint& textualInputRoot = source.GetInputRoot();
			const Client::IInputParameterRegistry& inputParamsReg = m_pGeneratorFactory->GetInputParameterRegistry();

			if (!inputRoot.Resolve(textualInputRoot, inputParamsReg, queryBlueprintForGlobalParamChecking, true))
			{
				return false;
			}

			ResolveInputs(inputRoot);

			return true;
		}

		const Shared::CTypeInfo& CGeneratorBlueprint::GetTypeOfItemsToGenerate() const
		{
			assert(m_pGeneratorFactory);
			return m_pGeneratorFactory->GetTypeOfItemsToGenerate();
		}

		Client::GeneratorUniquePtr CGeneratorBlueprint::InstantiateGenerator(const SQueryBlackboard& blackboard, Shared::CUqsString& error) const
		{
			assert(m_pGeneratorFactory);

			//
			// create the input parameters (they will get filled by the function calls below)
			//

			Client::ParamsHolderUniquePtr pParamsHolder = m_pGeneratorFactory->GetParamsHolderFactory().CreateParamsHolder();
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
			const Client::IFunction::SExecuteContext execContext(0, blackboard, error, bExceptionOccurredDuringFunctionCalls);  // currentItemIndex == 0: this is just a dummy, as we're not iterating on items

			functionCalls.ExecuteAll(execContext, pParams, m_pGeneratorFactory->GetInputParameterRegistry());

			if (bExceptionOccurredDuringFunctionCalls)
			{
				return nullptr;
			}

			//
			// instantiate the generator with the input params
			//

			Client::GeneratorUniquePtr pGenerator = m_pGeneratorFactory->CreateGenerator(pParams);   // never returns NULL
			assert(pGenerator);
			return pGenerator;
		}

		void CGeneratorBlueprint::PrintToConsole(CLogger& logger) const
		{
			logger.Printf("Generator: %s [%s]", m_pGeneratorFactory->GetName(), m_pGeneratorFactory->GetTypeOfItemsToGenerate().name());
		}

	}
}
