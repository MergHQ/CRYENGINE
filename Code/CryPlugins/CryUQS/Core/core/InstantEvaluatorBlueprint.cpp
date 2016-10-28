// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "InstantEvaluatorBlueprint.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualInstantEvaluatorBlueprint
		//
		//===================================================================================

		CTextualInstantEvaluatorBlueprint::CTextualInstantEvaluatorBlueprint()
			: m_weight("1.0")
		{
		}

		void CTextualInstantEvaluatorBlueprint::SetEvaluatorName(const char* evaluatorName)
		{
			m_evaluatorName = evaluatorName;
		}

		ITextualInputBlueprint& CTextualInstantEvaluatorBlueprint::GetInputRoot()
		{
			return m_rootInput;
		}

		const char* CTextualInstantEvaluatorBlueprint::GetEvaluatorName() const
		{
			return m_evaluatorName.c_str();
		}

		const ITextualInputBlueprint& CTextualInstantEvaluatorBlueprint::GetInputRoot() const
		{
			return m_rootInput;
		}

		void CTextualInstantEvaluatorBlueprint::SetWeight(const char* weight)
		{
			m_weight = weight;
		}

		const char* CTextualInstantEvaluatorBlueprint::GetWeight() const
		{
			return m_weight.c_str();
		}

		void CTextualInstantEvaluatorBlueprint::SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr)
		{
			m_pSyntaxErrorCollector = std::move(ptr);
		}

		datasource::ISyntaxErrorCollector* CTextualInstantEvaluatorBlueprint::GetSyntaxErrorCollector() const
		{
			return m_pSyntaxErrorCollector.get();
		}

		//===================================================================================
		//
		// CInstantEvaluatorBlueprint
		//
		//===================================================================================

		CInstantEvaluatorBlueprint::CInstantEvaluatorBlueprint()
			: m_pInstantEvaluatorFactory(nullptr)
			, m_weight(1.0f)
		{}

		bool CInstantEvaluatorBlueprint::Resolve(const ITextualInstantEvaluatorBlueprint& source, const CQueryBlueprint& queryBlueprintForGlobalParamChecking)
		{
			const char* evaluatorName = source.GetEvaluatorName();

			m_pInstantEvaluatorFactory = g_hubImpl->GetInstantEvaluatorFactoryDatabase().FindFactoryByName(evaluatorName);
			if (!m_pInstantEvaluatorFactory)
			{
				if (datasource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Unknown InstantEvaluatorFactory '%s'", evaluatorName);
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
			const client::IInputParameterRegistry& inputParamsReg = m_pInstantEvaluatorFactory->GetInputParameterRegistry();

			if (!inputRoot.Resolve(textualInputRoot, inputParamsReg, queryBlueprintForGlobalParamChecking, false))
			{
				return false;
			}

			ResolveInputs(inputRoot);

			// if we're here, then the only parse failure that could have happened is a bad weight, so the final result depends only on that particular outcome

			return parsedWeightSuccessfully;
		}

		client::IInstantEvaluatorFactory& CInstantEvaluatorBlueprint::GetFactory() const
		{
			assert(m_pInstantEvaluatorFactory);
			return *m_pInstantEvaluatorFactory;
		}

		float CInstantEvaluatorBlueprint::GetWeight() const
		{
			return m_weight;
		}

		void CInstantEvaluatorBlueprint::PrintToConsole(CLogger& logger, const char* messagePrefix) const
		{
			const char* cost;
			const char* modality;

			switch (m_pInstantEvaluatorFactory->GetCostCategory())
			{
			case client::IInstantEvaluatorFactory::ECostCategory::Cheap:
				cost = "cheap";
				break;

			case client::IInstantEvaluatorFactory::ECostCategory::Expensive:
				cost = "expensive";
				break;

			default:
				cost = "? (programming error)";
				break;
			}

			switch (m_pInstantEvaluatorFactory->GetEvaluationModality())
			{
			case client::IInstantEvaluatorFactory::EEvaluationModality::Testing:
				modality = "tester";
				break;

			case client::IInstantEvaluatorFactory::EEvaluationModality::Scoring:
				modality = "scorer";
				break;

			default:
				modality = "? (programming error)";
				break;
			}

			logger.Printf("%s%s [%s %s] (weight = %f)", messagePrefix, m_pInstantEvaluatorFactory->GetName(), cost, modality, m_weight);
		}

	}
}
