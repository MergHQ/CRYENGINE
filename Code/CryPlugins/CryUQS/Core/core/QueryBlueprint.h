// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualQueryBlueprint
		//
		//===================================================================================

		class CTextualQueryBlueprint : public ITextualQueryBlueprint
		{
		public:
			explicit                                                   CTextualQueryBlueprint();
			                                                           ~CTextualQueryBlueprint();

			///////////////////////////////////////////////////////////////////////////////////
			//
			// called by a loader that reads from an abstract data source to build the blueprint in textual form
			//
			///////////////////////////////////////////////////////////////////////////////////

			virtual void                                               SetName(const char* name) override;
			virtual void                                               SetQueryFactoryName(const char* factoryName) override;
			virtual void                                               SetMaxItemsToKeepInResultSet(const char* maxItems) override;
			virtual void                                               SetExpectedShuttleType(const char* shuttleTypeName) override;
			virtual ITextualGlobalConstantParamsBlueprint&             GetGlobalConstantParams() override;
			virtual ITextualGlobalRuntimeParamsBlueprint&              GetGlobalRuntimeParams() override;
			virtual ITextualGeneratorBlueprint&                        SetGenerator() override;
			virtual ITextualInstantEvaluatorBlueprint&                 AddInstantEvaluator() override;
			virtual ITextualDeferredEvaluatorBlueprint&                AddDeferredEvaluator() override;
			virtual ITextualQueryBlueprint&                            AddChild() override;

			///////////////////////////////////////////////////////////////////////////////////
			//
			// called by CQueryBlueprint::Resolve()
			//
			///////////////////////////////////////////////////////////////////////////////////

			virtual const char*                                        GetName() const override;
			virtual const char*                                        GetQueryFactoryName() const override;
			virtual const char*                                        GetMaxItemsToKeepInResultSet() const override;
			virtual const shared::IUqsString*                          GetExpectedShuttleType() const override;
			virtual const ITextualGlobalConstantParamsBlueprint&       GetGlobalConstantParams() const override;
			virtual const ITextualGlobalRuntimeParamsBlueprint&        GetGlobalRuntimeParams() const override;
			virtual const ITextualGeneratorBlueprint*                  GetGenerator() const override;
			virtual size_t                                             GetInstantEvaluatorCount() const override;
			virtual const ITextualInstantEvaluatorBlueprint&           GetInstantEvaluator(size_t index) const override;
			virtual size_t                                             GetDeferredEvaluatorCount() const override;
			virtual const ITextualDeferredEvaluatorBlueprint&          GetDeferredEvaluator(size_t index) const override;
			virtual size_t                                             GetChildCount() const override;
			virtual const ITextualQueryBlueprint&                      GetChild(size_t index) const override;

			//
			// syntax error collector
			//

			virtual void                                               SetSyntaxErrorCollector(datasource::SyntaxErrorCollectorUniquePtr ptr) override;
			virtual datasource::ISyntaxErrorCollector*                 GetSyntaxErrorCollector() const override;

		private:
			                                                           UQS_NON_COPYABLE(CTextualQueryBlueprint);

		private:
			string                                                     m_name;
			string                                                     m_queryFactoryName;
			string                                                     m_maxItemsToKeepInResultSet;
			shared::CUqsString                                         m_expectedShuttleType;
			bool                                                       m_bExpectedShuttleTypeHasBeenProvided;
			CTextualGlobalConstantParamsBlueprint                      m_globalConstantParams;
			CTextualGlobalRuntimeParamsBlueprint                       m_globalRuntimeParams;
			std::unique_ptr<CTextualGeneratorBlueprint>                m_pGenerator;
			std::vector<CTextualInstantEvaluatorBlueprint*>            m_instantEvaluators;
			std::vector<CTextualDeferredEvaluatorBlueprint*>           m_deferredEvaluators;
			datasource::SyntaxErrorCollectorUniquePtr                  m_pSyntaxErrorCollector;
			std::vector<CTextualQueryBlueprint*>                       m_children;
		};

		//===================================================================================
		//
		// CQueryBlueprint
		//
		//===================================================================================

		class CQueryBlueprint : public IQueryBlueprint
		{
		public:
			explicit                                           CQueryBlueprint();
			                                                   ~CQueryBlueprint();

			// IQueryBlueprint
			virtual const char*                                GetName() const override;
			virtual void                                       VisitRuntimeParams(client::IQueryBlueprintRuntimeParamVisitor& visitor) const override;
			virtual const shared::CTypeInfo&                   GetOutputType() const override;
			// ~IQueryBlueprint

			// - called when resolving a textual blueprint into its compiled counterpart
			// - after that, the blueprint can be instantiated as "live" Query at any time
			bool                                               Resolve(const ITextualQueryBlueprint& source);

			const CQueryBlueprint*                             GetParent() const;
			size_t                                             GetChildCount() const;
			std::shared_ptr<const CQueryBlueprint>             GetChild(size_t index) const;

			QueryBaseUniquePtr                                 CreateQuery(const CQueryBase::SCtorContext& ctorContext) const;

			// these are called during the instantiation of a "live" Query
			int                                                GetMaxItemsToKeepInResultSet() const;
			const shared::CTypeInfo*                           GetExpectedShuttleType() const;
			const CGlobalConstantParamsBlueprint&              GetGlobalConstantParamsBlueprint() const;
			const CGlobalRuntimeParamsBlueprint&               GetGlobalRuntimeParamsBlueprint() const;
			const CGeneratorBlueprint*                         GetGeneratorBlueprint() const;
			const std::vector<CInstantEvaluatorBlueprint*>&    GetInstantEvaluatorBlueprints() const;
			const std::vector<CDeferredEvaluatorBlueprint*>&   GetDeferredEvaluatorBlueprints() const;
			// TODO: secondary-generator blueprints?
			bool                                               CheckPresenceAndTypeOfGlobalRuntimeParamsRecursively(const shared::IVariantDict& runtimeParamsToValidate, shared::CUqsString& error) const;

			// debugging: dumps query-blueprint details to the console
			void                                               PrintToConsole(CLogger& logger) const;

		private:
			                                                   UQS_NON_COPYABLE(CQueryBlueprint);

			void                                               GrabRuntimeParamsRecursively(std::map<string, client::IItemFactory*>& out) const;

		private:
			string                                             m_name;
			CQueryFactoryBase*                                 m_pQueryFactory;
			int                                                m_maxItemsToKeepInResultSet;		// item limitation gets only effective if this is > 0
			const shared::CTypeInfo*                           m_pExpectedShuttleType;
			CGlobalConstantParamsBlueprint                     m_globalConstantParams;
			CGlobalRuntimeParamsBlueprint                      m_globalRuntimeParams;
			std::unique_ptr<CGeneratorBlueprint>               m_pGenerator;
			std::vector<CInstantEvaluatorBlueprint*>           m_instantEvaluators;
			std::vector<CDeferredEvaluatorBlueprint*>          m_deferredEvaluators;
			std::vector<std::shared_ptr<CQueryBlueprint>>      m_children;
			const CQueryBlueprint*                             m_pParent;

			// TODO: secondary-generator blueprint(s)?
		};

	}
}
