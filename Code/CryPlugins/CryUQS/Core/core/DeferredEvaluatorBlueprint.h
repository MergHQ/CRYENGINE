// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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

		class CTextualDeferredEvaluatorBlueprint : public ITextualDeferredEvaluatorBlueprint
		{
		public:
			explicit                                             CTextualDeferredEvaluatorBlueprint();

			virtual void                                         SetWeight(float weight) override;
			virtual float                                        GetWeight() const override;

			// called by a loader that reads from an abstract data source to build the blueprint in textual form
			virtual void                                         SetEvaluatorName(const char* szEvaluatorName) override;
			virtual ITextualInputBlueprint&                      GetInputRoot() override;

			// called by CDeferredEvaluatorBlueprint::Resolve()
			virtual const char*                                  GetEvaluatorName() const override;
			virtual const ITextualInputBlueprint&                GetInputRoot() const override;

			virtual void                                         SetSyntaxErrorCollector(DataSource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) override;
			virtual DataSource::ISyntaxErrorCollector*           GetSyntaxErrorCollector() const override;

		private:
			                                                     UQS_NON_COPYABLE(CTextualDeferredEvaluatorBlueprint);

		private:
			string                                               m_evaluatorName;
			float                                                m_weight;
			CTextualInputBlueprint                               m_rootInput;
			DataSource::SyntaxErrorCollectorUniquePtr            m_pSyntaxErrorCollector;
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
			Client::IDeferredEvaluatorFactory&            GetFactory() const;
			float                                         GetWeight() const;
			void                                          PrintToConsole(CLogger& logger, const char* szMessagePrefix) const;

		private:
			                                              UQS_NON_COPYABLE(CDeferredEvaluatorBlueprint);

		private:
			Client::IDeferredEvaluatorFactory*            m_pDeferredEvaluatorFactory;
			float                                         m_weight;
		};

	}
}
