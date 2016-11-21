// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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

		class CTextualInstantEvaluatorBlueprint : public ITextualInstantEvaluatorBlueprint
		{
		public:
			explicit                                            CTextualInstantEvaluatorBlueprint();

			virtual void                                        SetWeight(const char* weight) override;
			virtual const char*                                 GetWeight() const override;

			// called by a loader that reads from an abstract data source to build the blueprint in textual form
			virtual void                                        SetEvaluatorName(const char* evaluatorName) override;
			virtual ITextualInputBlueprint&                     GetInputRoot() override;

			// called by CInstantEvaluatorBlueprint::Resolve()
			virtual const char*                                 GetEvaluatorName() const override;
			virtual const ITextualInputBlueprint&               GetInputRoot() const override;

			virtual void                                        SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr) override;
			virtual datasource::ISyntaxErrorCollector*          GetSyntaxErrorCollector() const override;

		private:
			                                                    UQS_NON_COPYABLE(CTextualInstantEvaluatorBlueprint);

		private:
			string                                              m_evaluatorName;
			string                                              m_weight;
			CTextualInputBlueprint                              m_rootInput;
			datasource::SyntaxErrorCollectorUniquePtr           m_pSyntaxErrorCollector;
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
			client::IInstantEvaluatorFactory&           GetFactory() const;
			float                                       GetWeight() const;
			void                                        PrintToConsole(CLogger& logger, const char* messagePrefix) const;

		private:
			                                            UQS_NON_COPYABLE(CInstantEvaluatorBlueprint);

		private:
			client::IInstantEvaluatorFactory*           m_pInstantEvaluatorFactory;
			float                                       m_weight;
		};

	}
}
