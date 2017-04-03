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
			, m_bNegateDiscard(false)
			, m_scoreTransform(CScoreTransformFactory::GetDefaultScoreTransformFactory().GetName())
		{
		}

		void CTextualInstantEvaluatorBlueprint::SetEvaluatorName(const char* szEvaluatorName)
		{
			m_evaluatorName = szEvaluatorName;
		}

		ITextualInputBlueprint& CTextualInstantEvaluatorBlueprint::GetInputRoot()
		{
			return m_rootInput;
		}

		void CTextualInstantEvaluatorBlueprint::SetScoreTransform(const char* szScoreTransform)
		{
			m_scoreTransform = szScoreTransform;
		}

		const char* CTextualInstantEvaluatorBlueprint::GetScoreTransform() const
		{
			return m_scoreTransform.c_str();
		}

		void CTextualInstantEvaluatorBlueprint::SetNegateDiscard(bool bNegateDiscard)
		{
			m_bNegateDiscard = bNegateDiscard;
		}

		bool CTextualInstantEvaluatorBlueprint::GetNegateDiscard() const
		{
			return m_bNegateDiscard;
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

		void CTextualInstantEvaluatorBlueprint::SetSyntaxErrorCollector(DataSource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector)
		{
			m_pSyntaxErrorCollector = std::move(pSyntaxErrorCollector);
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
			const char* szEvaluatorName = source.GetEvaluatorName();

			m_pInstantEvaluatorFactory = g_pHub->GetInstantEvaluatorFactoryDatabase().FindFactoryByName(szEvaluatorName);
			if (!m_pInstantEvaluatorFactory)
			{
				if (DataSource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Unknown InstantEvaluatorFactory '%s'", szEvaluatorName);
				}
				return false;
			}

			m_weight = source.GetWeight();

			const char* szScoreTransform = source.GetScoreTransform();
			if (szScoreTransform[0] != '\0')
			{
				const IScoreTransformFactory* pScoreTransformFactory = g_pHub->GetScoreTransformFactoryDatabase().FindFactoryByName(szScoreTransform);

				if (!pScoreTransformFactory)
				{
					if (DataSource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
					{
						pSE->AddErrorMessage("Unknown ScoreTransformFactory: '%s'", szScoreTransform);
					}
					return false;
				}

				m_evaluationResultTransform.SetScoreTransformType(pScoreTransformFactory->GetScoreTransformType());
			}
			m_evaluationResultTransform.SetNegateDiscard(source.GetNegateDiscard());

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

		const CEvaluationResultTransform& CInstantEvaluatorBlueprint::GetEvaluationResultTransform() const
		{
			return m_evaluationResultTransform;
		}

		void CInstantEvaluatorBlueprint::PrintToConsole(CLogger& logger, const char* szMessagePrefix) const
		{
			const char* szCost;
			const char* szModality;

			switch (m_pInstantEvaluatorFactory->GetCostCategory())
			{
			case Client::IInstantEvaluatorFactory::ECostCategory::Cheap:
				szCost = "cheap";
				break;

			case Client::IInstantEvaluatorFactory::ECostCategory::Expensive:
				szCost = "expensive";
				break;

			default:
				szCost = "? (programming error)";
				break;
			}

			switch (m_pInstantEvaluatorFactory->GetEvaluationModality())
			{
			case Client::IInstantEvaluatorFactory::EEvaluationModality::Testing:
				szModality = "tester";
				break;

			case Client::IInstantEvaluatorFactory::EEvaluationModality::Scoring:
				szModality = "scorer";
				break;

			default:
				szModality = "? (programming error)";
				break;
			}

			logger.Printf("%s%s [%s %s] (weight = %f)", szMessagePrefix, m_pInstantEvaluatorFactory->GetName(), szCost, szModality, m_weight);
		}

	}
}
