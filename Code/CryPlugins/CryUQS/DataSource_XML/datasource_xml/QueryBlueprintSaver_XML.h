// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace datasource_xml
	{

		//===================================================================================
		//
		// CQueryBlueprintSaver_XML
		//
		//===================================================================================

		class CQueryBlueprintSaver_XML : public datasource::IQueryBlueprintSaver
		{
		public:
			explicit                                    CQueryBlueprintSaver_XML(const char* xmlFileNameToSaveTo);
			virtual bool                                SaveTextualQueryBlueprint(const core::ITextualQueryBlueprint& queryBlueprintToSave, shared::IUqsString& error) override;

		private:
			void                                        SaveQueryElement(const XmlNodeRef& queryElementToSaveTo);
			void                                        SaveGlobalParamsElement(const XmlNodeRef& globalParamsElementToSaveTo);
			void                                        SaveGeneratorElement(const XmlNodeRef& generatorElementToSaveTo);
			void                                        SaveInstantEvaluatorElement(const XmlNodeRef& instantEvaluatorElementToSaveTo, const core::ITextualInstantEvaluatorBlueprint& instantEvaluatorBP);
			void                                        SaveDeferredEvaluatorElement(const XmlNodeRef& deferredEvaluatorElementToSaveTo, const core::ITextualDeferredEvaluatorBlueprint& deferredEvaluatorBP);
			void                                        SaveFunctionElement(const XmlNodeRef& functionElementToSaveTo, const core::ITextualInputBlueprint& parentInput);
			void                                        SaveInputElement(const XmlNodeRef& inputElementToSaveTo, const core::ITextualInputBlueprint& inputBP);

		private:
			string                                      m_xmlFileNameToSaveTo;
			XmlNodeRef                                  m_queryElementToSaveTo;
			const core::ITextualQueryBlueprint*         m_query;	// points to what was passed in to SaveTextualQueryBlueprint() throughout the saving process
		};

	}
}
