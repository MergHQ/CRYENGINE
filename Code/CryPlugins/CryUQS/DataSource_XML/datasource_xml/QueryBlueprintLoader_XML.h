// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace datasource_xml
	{

		class CXMLDataErrorCollector;

		//===================================================================================
		//
		// CQueryBlueprintLoader_XML
		//
		//===================================================================================

		class CQueryBlueprintLoader_XML : public datasource::IQueryBlueprintLoader
		{
		public:
			explicit                                  CQueryBlueprintLoader_XML(const char* queryName, const char* xmlFilePath, const std::shared_ptr<CXMLDataErrorCollector>& dataErrorCollector);
			virtual bool                              LoadTextualQueryBlueprint(core::ITextualQueryBlueprint& out, shared::IUqsString& error) override;

		private:
			bool                                      ParseQueryElement(const XmlNodeRef& queryElement, shared::IUqsString& error);
			bool                                      ParseGlobalParamsElement(const XmlNodeRef& globalParamsElement, shared::IUqsString& error);
			bool                                      ParseGeneratorElement(const XmlNodeRef& generatorElement, shared::IUqsString& error);
			bool                                      ParseInstantEvaluatorElement(const XmlNodeRef& instantEvaluatorElement, shared::IUqsString& error);
			bool                                      ParseDeferredEvaluatorElement(const XmlNodeRef& deferredEvaluatorElement, shared::IUqsString& error);
			bool                                      ParseFunctionElement(const XmlNodeRef& functionElement, core::ITextualInputBlueprint& parentInput, shared::IUqsString& error);
			bool                                      ParseInputElement(const XmlNodeRef& inputElement, core::ITextualInputBlueprint& parentInput, shared::IUqsString& error);

		private:
			string                                    m_queryName;
			string                                    m_xmlFilePath;
			XmlNodeRef                                m_queryElement;
			std::shared_ptr<CXMLDataErrorCollector>   m_dataErrorCollector;
			core::ITextualQueryBlueprint*             m_query;	// points to what was passed in to LoadTextualQueryBlueprint() throughout the loading process
		};

	}
}
