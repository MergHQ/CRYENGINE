// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualGlobalRuntimeParamsBlueprint
		//
		//===================================================================================

		class CTextualGlobalRuntimeParamsBlueprint : public ITextualGlobalRuntimeParamsBlueprint
		{
		public:
			explicit                                        CTextualGlobalRuntimeParamsBlueprint();

			// ITextualGlobalRuntimeParamsBlueprint
			virtual void                                    AddParameter(const char* name, const char* type, bool bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) override;
			virtual size_t                                  GetParameterCount() const override;
			virtual SParameterInfo                          GetParameter(size_t index) const override;
			// ~ITextualGlobalRuntimeParamsBlueprint

		private:
			struct SStoredParameterInfo
			{
				explicit                                    SStoredParameterInfo(const char* _name, const char *_type, bool _bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector);
				explicit                                    SStoredParameterInfo(SStoredParameterInfo&& other);
				string                                      name;
				string                                      type;
				bool                                        bAddToDebugRenderWorld;
				datasource::SyntaxErrorCollectorUniquePtr   pSyntaxErrorCollector;
			};

		private:
			                                                UQS_NON_COPYABLE(CTextualGlobalRuntimeParamsBlueprint);

		private:
			std::vector<SStoredParameterInfo>               m_parameters;
		};

		inline CTextualGlobalRuntimeParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(const char* _name, const char *_type, bool _bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector)
			: name(_name)
			, type(_type)
			, bAddToDebugRenderWorld(_bAddToDebugRenderWorld)
			, pSyntaxErrorCollector(std::move(_pSyntaxErrorCollector))
		{}

		inline CTextualGlobalRuntimeParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(SStoredParameterInfo&& other)
			: name(std::move(other.name))
			, type(std::move(other.type))
			, bAddToDebugRenderWorld(std::move(other.bAddToDebugRenderWorld))
			, pSyntaxErrorCollector(std::move(other.pSyntaxErrorCollector))
		{}

		//===================================================================================
		//
		// CGlobalRuntimeParamsBlueprint
		//
		//===================================================================================

		class CGlobalRuntimeParamsBlueprint
		{
		public:
			struct SParamInfo
			{
				explicit                                         SParamInfo(client::IItemFactory* _pItemFactory, bool _bAddToDebugRenderWorld);
				client::IItemFactory*                            pItemFactory;
				bool                                             bAddToDebugRenderWorld;
			};

		public:
			explicit                                             CGlobalRuntimeParamsBlueprint();

			bool                                                 Resolve(const ITextualGlobalRuntimeParamsBlueprint& source, const CQueryBlueprint* pParentQueryBlueprint);
			const std::map<string, SParamInfo>&                  GetParams() const;
			void                                                 PrintToConsole(CLogger& logger) const;

		private:
			                                                     UQS_NON_COPYABLE(CGlobalRuntimeParamsBlueprint);
			const client::IItemFactory*                          FindItemFactoryByParamNameInParentRecursively(const char* paramNameToSearchFor, const CQueryBlueprint& parentalQueryBlueprint) const;

		private:
			std::map<string, SParamInfo>                         m_runtimeParameters;
		};

		inline CGlobalRuntimeParamsBlueprint::SParamInfo::SParamInfo(client::IItemFactory* _pItemFactory, bool _bAddToDebugRenderWorld)
			: pItemFactory(_pItemFactory)
			, bAddToDebugRenderWorld(_bAddToDebugRenderWorld)
		{}

	}
}
