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

			virtual void                                    AddParameter(const char* name, const char* type, datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) override;
			virtual size_t                                  GetParameterCount() const override;
			virtual SParameterInfo                          GetParameter(size_t index) const override;

		private:
			struct SStoredParameterInfo
			{
				explicit                                    SStoredParameterInfo(const char* _name, const char *_type, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector);
				explicit                                    SStoredParameterInfo(SStoredParameterInfo&& other);
				string                                      name;
				string                                      type;
				datasource::SyntaxErrorCollectorUniquePtr   pSyntaxErrorCollector;
			};

		private:
			                                                UQS_NON_COPYABLE(CTextualGlobalRuntimeParamsBlueprint);

		private:
			std::vector<SStoredParameterInfo>               m_parameters;
		};

		inline CTextualGlobalRuntimeParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(const char* _name, const char *_type, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector)
			: name(_name)
			, type(_type)
			, pSyntaxErrorCollector(std::move(_pSyntaxErrorCollector))
		{}

		inline CTextualGlobalRuntimeParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(SStoredParameterInfo&& other)
			: name(std::move(other.name))
			, type(std::move(other.type))
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
			explicit                                             CGlobalRuntimeParamsBlueprint();

			bool                                                 Resolve(const ITextualGlobalRuntimeParamsBlueprint& source, const CQueryBlueprint* pParentQueryBlueprint);
			const std::map<string, client::IItemFactory*>&       GetParams() const;
			void                                                 PrintToConsole(CLogger& logger) const;

		private:
			                                                     UQS_NON_COPYABLE(CGlobalRuntimeParamsBlueprint);
			const client::IItemFactory*                          FindItemFactoryByParamNameInParentRecursively(const char* paramNameToSearchFor, const CQueryBlueprint& parentalQueryBlueprint) const;

		private:
			std::map<string, client::IItemFactory*>              m_runtimeParameters;
		};

	}
}
