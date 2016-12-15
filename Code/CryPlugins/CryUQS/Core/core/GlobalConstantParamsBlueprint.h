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

			virtual void                                  AddParameter(const char* name, const char* type, const char* value, datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector) override;
			virtual size_t                                GetParameterCount() const override;
			virtual SParameterInfo                        GetParameter(size_t index) const override;

		private:
			struct SStoredParameterInfo
			{
				explicit                                  SStoredParameterInfo(const char* _name, const char *_type, const char* _value, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector);
				explicit                                  SStoredParameterInfo(SStoredParameterInfo&& other);
				string                                    name;
				string                                    type;
				string                                    value;
				datasource::SyntaxErrorCollectorUniquePtr pSyntaxErrorCollector;
			};

		private:
			                                              UQS_NON_COPYABLE(CTextualGlobalConstantParamsBlueprint);

		private:
			std::vector<SStoredParameterInfo>             m_parameters;
		};

		inline CTextualGlobalConstantParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(const char* _name, const char *_type, const char* _value, datasource::SyntaxErrorCollectorUniquePtr _pSyntaxErrorCollector)
			: name(_name)
			, type(_type)
			, value(_value)
			, pSyntaxErrorCollector(std::move(_pSyntaxErrorCollector))
		{}

		inline CTextualGlobalConstantParamsBlueprint::SStoredParameterInfo::SStoredParameterInfo(SStoredParameterInfo&& other)
			: name(std::move(other.name))
			, type(std::move(other.type))
			, value(std::move(other.value))
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
			explicit                            CGlobalConstantParamsBlueprint();

			bool                                Resolve(const ITextualGlobalConstantParamsBlueprint& source);
			const shared::CVariantDict&         GetParams() const;
			void                                PrintToConsole(CLogger& logger) const;

		private:
			                                    UQS_NON_COPYABLE(CGlobalConstantParamsBlueprint);

		private:
			shared::CVariantDict                m_constantParams;
		};

	}
}
