// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "InstantEvaluatorBlueprint.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CTextualInstantEvaluatorBlueprint
		//
		//===================================================================================

		CTextualInstantEvaluatorBlueprint::CTextualInstantEvaluatorBlueprint()
			: m_weight(1.0)
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

		void CTextualInstantEvaluatorBlueprint::SetWeight(float weight)
		{
			m_weight = weight;
		}

		float CTextualInstantEvaluatorBlueprint::GetWeight() const
		{
			return m_weight;
		}

		void CTextualInstantEvaluatorBlueprint::SetSyntaxErrorCollector(DataSource::SyntaxErrorCollectorUniquePtr ptr)
		{
			m_pSyntaxErrorCollector = std::move(ptr);
		}

		DataSource::ISyntaxErrorCollector* CTextualInstantEvaluatorBlueprint::GetSyntaxErrorCollector() const
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
				if (DataSource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Unknown InstantEvaluatorFactory '%s'", evaluatorName);
				}
				return false;
			}

			m_weight = source.GetWeight();

			CInputBlueprint inputRoot;
			const ITextualInputBlueprint& textualInputRoot = source.GetInputRoot();
			const Client::IInputParameterRegistry& inputParamsReg = m_pInstantEvaluatorFactory->GetInputParameterRegistry();

			if (!inputRoot.Resolve(textualInputRoot, inputParamsReg, queryBlueprintForGlobalParamChecking, false))
			{
				return false;
			}

			ResolveInputs(inputRoot);

			return true;
		}

		Client::IInstantEvaluatorFactory& CInstantEvaluatorBlueprint::GetFactory() const
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
			case Client::IInstantEvaluatorFactory::ECostCategory::Cheap:
				cost = "cheap";
				break;

			case Client::IInstantEvaluatorFactory::ECostCategory::Expensive:
				cost = "expensive";
				break;

			default:
				cost = "? (programming error)";
				break;
			}

			switch (m_pInstantEvaluatorFactory->GetEvaluationModality())
			{
			case Client::IInstantEvaluatorFactory::EEvaluationModality::Testing:
				modality = "tester";
				break;

			case Client::IInstantEvaluatorFactory::EEvaluationModality::Scoring:
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
