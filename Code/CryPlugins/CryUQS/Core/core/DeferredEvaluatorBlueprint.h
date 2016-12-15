// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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

		class CTextualDeferredEvaluatorBlueprint : public ITextualDeferredEvaluatorBlueprint
		{
		public:
			explicit                                             CTextualDeferredEvaluatorBlueprint();

			virtual void                                         SetWeight(const char* weight) override;
			virtual const char*                                  GetWeight() const override;

			// called by a loader that reads from an abstract data source to build the blueprint in textual form
			virtual void                                         SetEvaluatorName(const char* evaluatorName) override;
			virtual ITextualInputBlueprint&                      GetInputRoot() override;

			// called by CDeferredEvaluatorBlueprint::Resolve()
			virtual const char*                                  GetEvaluatorName() const override;
			virtual const ITextualInputBlueprint&                GetInputRoot() const override;

			virtual void                                         SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr) override;
			virtual datasource::ISyntaxErrorCollector*           GetSyntaxErrorCollector() const override;

		private:
			                                                     UQS_NON_COPYABLE(CTextualDeferredEvaluatorBlueprint);

		private:
			string                                               m_evaluatorName;
			string                                               m_weight;
			CTextualInputBlueprint                               m_rootInput;
			datasource::SyntaxErrorCollectorUniquePtr            m_pSyntaxErrorCollector;
		};

		//===================================================================================
		//
		// CDeferredEvaluatorBlueprint
		//
		//===================================================================================

		class CDeferredEvaluatorBlueprint : public CBlueprintWithInputs
		{
		public:
			explicit                                      CDeferredEvaluatorBlueprint();

			bool                                          Resolve(const ITextualDeferredEvaluatorBlueprint& source, const CQueryBlueprint& queryBlueprintForGlobalParamChecking);
			client::IDeferredEvaluatorFactory&            GetFactory() const;
			float                                         GetWeight() const;
			void                                          PrintToConsole(CLogger& logger, const char* messagePrefix) const;

		private:
			                                              UQS_NON_COPYABLE(CDeferredEvaluatorBlueprint);

		private:
			client::IDeferredEvaluatorFactory*            m_pDeferredEvaluatorFactory;
			float                                         m_weight;
		};

	}
}
