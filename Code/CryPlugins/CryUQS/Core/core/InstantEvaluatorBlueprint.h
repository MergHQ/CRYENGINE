// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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

		class CTextualInstantEvaluatorBlueprint : public ITextualInstantEvaluatorBlueprint
		{
		public:
			explicit                                            CTextualInstantEvaluatorBlueprint();

			virtual void                                        SetWeight(float weight) override;
			virtual float                                       GetWeight() const override;

			// called by a loader that reads from an abstract data source to build the blueprint in textual form
			virtual void                                        SetEvaluatorName(const char* szEvaluatorName) override;
			virtual ITextualInputBlueprint&                     GetInputRoot() override;

			// called by CInstantEvaluatorBlueprint::Resolve()
			virtual const char*                                 GetEvaluatorName() const override;
			virtual const ITextualInputBlueprint&               GetInputRoot() const override;

			virtual void                                        SetScoreTransform(const char* szScoreTransform) override;
			virtual const char*                                 GetScoreTransform() const override;

			virtual void                                        SetNegateDiscard(bool bNegateDiscard) override;
			virtual bool                                        GetNegateDiscard() const override;

			virtual void                                        SetSyntaxErrorCollector(DataSource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) override;
			virtual DataSource::ISyntaxErrorCollector*          GetSyntaxErrorCollector() const override;

		private:
			                                                    UQS_NON_COPYABLE(CTextualInstantEvaluatorBlueprint);

		private:
			string                                              m_evaluatorName;
			float                                               m_weight;
			CTextualInputBlueprint                              m_rootInput;
			string                                              m_scoreTransform;
			bool                                                m_bNegateDiscard;
			DataSource::SyntaxErrorCollectorUniquePtr           m_pSyntaxErrorCollector;
		};

		//===================================================================================
		//
		// CInstantEvaluatorBlueprint
		//
		//===================================================================================

		class CInstantEvaluatorBlueprint : public CBlueprintWithInputs
		{
		public:
			explicit                                    CInstantEvaluatorBlueprint();

			bool                                        Resolve(const ITextualInstantEvaluatorBlueprint& source, const CQueryBlueprint& queryBlueprintForGlobalParamChecking);
			Client::IInstantEvaluatorFactory&           GetFactory() const;
			float                                       GetWeight() const;
			const CEvaluationResultTransform&           GetEvaluationResultTransform() const;
			void                                        PrintToConsole(CLogger& logger, const char* szMessagePrefix) const;

		private:
			                                            UQS_NON_COPYABLE(CInstantEvaluatorBlueprint);

		private:
			Client::IInstantEvaluatorFactory*           m_pInstantEvaluatorFactory;
			float                                       m_weight;
			CEvaluationResultTransform                  m_evaluationResultTransform;
		};

	}
}
