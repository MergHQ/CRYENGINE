// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualGeneratorBlueprint
		//
		//===================================================================================

		class CTextualGeneratorBlueprint : public ITextualGeneratorBlueprint
		{
		public:
			explicit                                             CTextualGeneratorBlueprint();

			// called by a loader that reads from an abstract data source to build the blueprint in textual form
			virtual void                                         SetGeneratorName(const char* generatorName) override;
			virtual ITextualInputBlueprint&                      GetInputRoot() override;

			// called by CGeneratorBlueprint::Resolve()
			virtual const char*                                  GetGeneratorName() const override;
			virtual const ITextualInputBlueprint&                GetInputRoot() const override;

			virtual void                                         SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr) override;
			virtual datasource::ISyntaxErrorCollector*           GetSyntaxErrorCollector() const override;

		private:
			                                                     UQS_NON_COPYABLE(CTextualGeneratorBlueprint);

		private:
			string                                               m_generatorName;
			CTextualInputBlueprint                               m_rootInput;
			datasource::SyntaxErrorCollectorUniquePtr            m_pSyntaxErrorCollector;
		};

		//===================================================================================
		//
		// CGeneratorBlueprint
		//
		//===================================================================================

		class CGeneratorBlueprint : public CBlueprintWithInputs
		{
		public:
			explicit                          CGeneratorBlueprint();

			bool                              Resolve(const ITextualGeneratorBlueprint& source, const CQueryBlueprint& queryBlueprintForGlobalParamChecking);
			const shared::CTypeInfo&          GetTypeOfItemsToGenerate() const;
			client::GeneratorUniquePtr        InstantiateGenerator(const SQueryBlackboard& blackboard, shared::CUqsString& error) const;
			void                              PrintToConsole(CLogger& logger) const;

		private:
			                                  UQS_NON_COPYABLE(CGeneratorBlueprint);

		private:
			client::IGeneratorFactory*        m_pGeneratorFactory;
		};

	}
}
