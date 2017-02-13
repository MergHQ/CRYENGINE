// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualGlobalConstantParamsBlueprint
		//
		//===================================================================================

		class CTextualGlobalConstantParamsBlueprint : public ITextualGlobalConstantParamsBlueprint
		{
		public:
			explicit                                      CTextualGlobalConstantParamsBlueprint();

			// ITextualGlobalConstantParamsBlueprint
			virtual void                                  AddParameter(const char* name, const char* type, const char* value, bool bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) override;
			virtual size_t                                GetParameterCount() const override;
			virtual SParameterInfo                        GetParameter(size_t index) const override;
			// ~ITextualGlobalConstantParamsBlueprint

		private:
			struct SStoredParameterInfo
			{
				explicit                                  SStoredParameterInfo(const char* _name, const char *_type, const char* _value, bool _bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector);
				explicit                                  SStoredParameterInfo(SStoredParameterInfo&& other);
				string                                    name;
				string                                    type;
				string                                    value;
				bool                                      bAddToDebugRenderWorld;
				datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector;
			};

		private:
			                                              UQS_NON_COPYABLE(CTextualGlobalConstantParamsBlueprint);

		private:
			std::vector<SStoredParameterInfo>             m_parameters;
		};

		inline CTextualGlobalConstantParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(const char* _name, const char *_type, const char* _value, bool _bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector)
			: name(_name)
			, type(_type)
			, value(_value)
			, bAddToDebugRenderWorld(_bAddToDebugRenderWorld)
			, pSyntaxErrorCollector(std::move(_pSyntaxErrorCollector))
		{}

		inline CTextualGlobalConstantParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(SStoredParameterInfo&& other)
			: name(std::move(other.name))
			, type(std::move(other.type))
			, value(std::move(other.value))
			, bAddToDebugRenderWorld(std::move(other.bAddToDebugRenderWorld))
			, pSyntaxErrorCollector(std::move(other.pSyntaxErrorCollector))
		{}

		//===================================================================================
		//
		// CGlobalConstantParamsBlueprint
		//
		//===================================================================================

		class CGlobalConstantParamsBlueprint
		{
		public:
			struct SParamInfo
			{
				explicit                        SParamInfo(client::IItemFactory* _pItemFactory, void* _pItem, bool _bAddToDebugRenderWorld);
				client::IItemFactory*           pItemFactory;
				void*                           pItem;
				bool                            bAddToDebugRenderWorld;
			};

		public:
			explicit                            CGlobalConstantParamsBlueprint();
			                                    ~CGlobalConstantParamsBlueprint();

			bool                                Resolve(const ITextualGlobalConstantParamsBlueprint& source);
			const std::map<string, SParamInfo>& GetParams() const;
			void                                AddSelfToDictAndReplace(shared::CVariantDict& out) const;
			void                                PrintToConsole(CLogger& logger) const;

		private:
			                                    UQS_NON_COPYABLE(CGlobalConstantParamsBlueprint);

		private:
			std::map<string, SParamInfo>        m_constantParams;
		};

		inline CGlobalConstantParamsBlueprint::SParamInfo::SParamInfo(client::IItemFactory* _pItemFactory, void* _pItem, bool _bAddToDebugRenderWorld)
			: pItemFactory(_pItemFactory)
			, pItem(_pItem)
			, bAddToDebugRenderWorld(_bAddToDebugRenderWorld)
		{}

	}
}
