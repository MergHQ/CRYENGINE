// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DeferredEvaluatorBlueprint.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CTextualDeferredEvaluatorBlueprint
		//
		//===================================================================================

		CTextualDeferredEvaluatorBlueprint::CTextualDeferredEvaluatorBlueprint()
			: m_weight(1.0)
			, m_bNegateDiscard(false)
			, m_scoreTransform(CScoreTransformFactory::GetDefaultScoreTransformFactory().GetName())
		{
		}

		void CTextualDeferredEvaluatorBlueprint::SetEvaluatorName(const char* szEvaluatorName)
		{
			m_evaluatorName = szEvaluatorName;
		}

		ITextualInputBlueprint& CTextualDeferredEvaluatorBlueprint::GetInputRoot()
		{
			return m_rootInput;
		}

		void CTextualDeferredEvaluatorBlueprint::SetScoreTransform(const char* szScoreTransform)
		{
			m_scoreTransform = szScoreTransform;
		}

		const char* CTextualDeferredEvaluatorBlueprint::GetScoreTransform() const
		{
			return m_scoreTransform.c_str();
		}

		void CTextualDeferredEvaluatorBlueprint::SetNegateDiscard(bool bNegateDiscard)
		{
			m_bNegateDiscard = bNegateDiscard;
		}

		bool CTextualDeferredEvaluatorBlueprint::GetNegateDiscard() const
		{
			return m_bNegateDiscard;
		}

		const char* CTextualDeferredEvaluatorBlueprint::GetEvaluatorName() const
		{
			return m_evaluatorName.c_str();
		}

		const ITextualInputBlueprint& CTextualDeferredEvaluatorBlueprint::GetInputRoot() const
		{
			return m_rootInput;
		}

		void CTextualDeferredEvaluatorBlueprint::SetWeight(float weight)
		{
			m_weight = weight;
		}

		float CTextualDeferredEvaluatorBlueprint::GetWeight() const
		{
			return m_weight;
		}

		void CTextualDeferredEvaluatorBlueprint::SetSyntaxErrorCollector(DataSource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector)
		{
			m_pSyntaxErrorCollector = std::move(pSyntaxErrorCollector);
		}

		DataSource::ISyntaxErrorCollector* CTextualDeferredEvaluatorBlueprint::GetSyntaxErrorCollector() const
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
			const char* szEvaluatorName = source.GetEvaluatorName();

			m_pDeferredEvaluatorFactory = g_pHub->GetDeferredEvaluatorFactoryDatabase().FindFactoryByName(szEvaluatorName);
			if (!m_pDeferredEvaluatorFactory)
			{
				if (DataSource::ISyntaxErrorCollector* pSE = source.GetSyntaxErrorCollector())
				{
					pSE->AddErrorMessage("Unknown DeferredEvaluatorFactory '%s'", szEvaluatorName);
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
			const Client::IInputParameterRegistry& inputParamsReg = m_pDeferredEvaluatorFactory->GetInputParameterRegistry();

			if (!inputRoot.Resolve(textualInputRoot, inputParamsReg, queryBlueprintForGlobalParamChecking, false))
			{
				return false;
			}

			ResolveInputs(inputRoot);

			return true;
		}

		Client::IDeferredEvaluatorFactory& CDeferredEvaluatorBlueprint::GetFactory() const
		{
			assert(m_pDeferredEvaluatorFactory);
			return *m_pDeferredEvaluatorFactory;
		}

		float CDeferredEvaluatorBlueprint::GetWeight() const
		{
			return m_weight;
		}

		const CEvaluationResultTransform& CDeferredEvaluatorBlueprint::GetEvaluationResultTransform() const
		{
			return m_evaluationResultTransform;
		}

		void CDeferredEvaluatorBlueprint::PrintToConsole(CLogger& logger, const char* szMessagePrefix) const
		{
			logger.Printf("%s%s (weight = %f)", szMessagePrefix, m_pDeferredEvaluatorFactory->GetName(), m_weight);
		}

	}
}
