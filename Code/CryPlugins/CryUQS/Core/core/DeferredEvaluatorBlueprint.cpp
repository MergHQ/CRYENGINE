// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DeferredEvaluatorBlueprint.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualDeferredEvaluatorBlueprint
		//
		//===================================================================================

		CTextualDeferredEvaluatorBlueprint::CTextualDeferredEvaluatorBlueprint()
			: m_weight("1.0")
		{
		}

		void CTextualDeferredEvaluatorBlueprint::SetEvaluatorName(const char* evaluatorName)
		{
			m_evaluatorName = evaluatorName;
		}

		ITextualInputBlueprint& CTextualDeferredEvaluatorBlueprint::GetInputRoot()
		{
			return m_rootInput;
		}

		const char* CTextualDeferredEvaluatorBlueprint::GetEvaluatorName() const
		{
			return m_evaluatorName.c_str();
		}

		const ITextualInputBlueprint& CTextualDeferredEvaluatorBlueprint::GetInputRoot() const
		{
			return m_rootInput;
		}

		void CTextualDeferredEvaluatorBlueprint::SetWeight(const char* weight)
		{
			m_weight = weight;
		}

		const char* CTextualDeferredEvaluatorBlueprint::GetWeight() const
		{
			return m_weight.c_str();
		}

		void CTextualDeferredEvaluatorBlueprint::SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr)
		{
			m_pSyntaxErrorCollector = std::move(ptr);
		}

		datasource::ISyntaxErrorCollector* CTextualDeferredEvaluatorBlueprint::GetSyntaxErrorCollector() const
		{
			return m_pSyntaxErrorCollector.get();
		}

		//===================================================================================
		//
		// CDeferredEvaluatorBlueprint
		//
		//===================================================================================

		CDeferredEvaluatorBlueprint::CDeferredEvaluatorBlueprint()
			: m_pDeferredEvaluatorFactory(nullptr)
			, m_weight(1.0f)
		{}

		bool CDeferredEvaluatorBlueprint::Resolve(const ITextualDeferredEvaluatorBlueprint& source, const CQueryBlueprint& queryBlueprintForGlobalParamChecking)
		{
			const char* evaluatorName = source.GetEvaluatorName();

			m_pDeferredEvaluatorFactory = g_hubImpl->GetDeferredEvaluatorFactoryDatabase().FindFactoryByName(evaluatorName);
			if (!m_pDeferredEvaluatorFactory)
			{
				if (datasource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Unknown DeferredEvaluatorFactory '%s'", evaluatorName);
				}
				return false;
			}

			bool parsedWeightSuccessfully = true;
			const char* weight = source.GetWeight();
			if (sscanf(weight, "%f", &m_weight) != 1)
			{
				if (datasource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Could not parse the weight from its textual representation into a float: '%s'", weight);
				}

				// having failed to parse the weight is not critical in the sense that it prevents parsing further data, but we need to remember its failure for the return value
				parsedWeightSuccessfully = false;
			}

			CInputBlueprint inputRoot;
			const ITextualInputBlueprint& textualInputRoot = source.GetInputRoot();
			const client::IInputParameterRegistry& inputParamsReg = m_pDeferredEvaluatorFactory->GetInputParameterRegistry();

			if (!inputRoot.Resolve(textualInputRoot, inputParamsReg, queryBlueprintForGlobalParamChecking, false))
			{
				return false;
			}

			ResolveInputs(inputRoot);

			// if we're here, then the only parse failure that could have happened is a bad weight, so the final result depends only on that particular outcome

			return parsedWeightSuccessfully;
		}

		client::IDeferredEvaluatorFactory& CDeferredEvaluatorBlueprint::GetFactory() const
		{
			assert(m_pDeferredEvaluatorFactory);
			return *m_pDeferredEvaluatorFactory;
		}

		float CDeferredEvaluatorBlueprint::GetWeight() const
		{
			return m_weight;
		}

		void CDeferredEvaluatorBlueprint::PrintToConsole(CLogger& logger, const char* messagePrefix) const
		{
			logger.Printf("%s%s (weight = %f)", messagePrefix, m_pDeferredEvaluatorFactory->GetName(), m_weight);
		}

	}
}
