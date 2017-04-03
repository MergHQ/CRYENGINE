// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "QueryBlueprintSaver_XML.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace DataSource_XML
	{

		CQueryBlueprintSaver_XML::CQueryBlueprintSaver_XML(const char* szXmlFileNameToSaveTo)
			: m_xmlFileNameToSaveTo(szXmlFileNameToSaveTo)
			, m_queryElementToSaveTo()
			, m_pQuery(nullptr)
		{
			// nothing
		}

		bool CQueryBlueprintSaver_XML::SaveTextualQueryBlueprint(const Core::ITextualQueryBlueprint& queryBlueprintToSave, Shared::IUqsString& error)
		{
			m_queryElementToSaveTo = gEnv->pSystem->CreateXmlNode("Query");

			m_pQuery = &queryBlueprintToSave;
			SaveQueryElement(m_queryElementToSaveTo);
			m_pQuery = nullptr;

			if (m_queryElementToSaveTo->saveToFile(m_xmlFileNameToSaveTo.c_str()))
			{
				return true;
			}
			else
			{
				error.Format("Could not save the query-blueprint to file '%s' (file might be read-only)", m_xmlFileNameToSaveTo.c_str());
				return false;
			}
		}

		void CQueryBlueprintSaver_XML::SaveQueryElement(const XmlNodeRef& queryElementToSaveTo)
		{
			assert(queryElementToSaveTo->isTag("Query"));

			// notice: we don't save the name of the query blueprint here, as the caller has a better understanding of how to deal with that
			//         (e. g. the name might be that of the file in which the query blueprint is stored)

			// "factory" attribute
			queryElementToSaveTo->setAttr("factory", m_pQuery->GetQueryFactoryName());

			// "maxItemsToKeepInResultSet" attribute
			queryElementToSaveTo->setAttr("maxItemsToKeepInResultSet", m_pQuery->GetMaxItemsToKeepInResultSet());

			// <GlobalParams>
			SaveGlobalParamsElement(queryElementToSaveTo->newChild("GlobalParams"));

			// <Generator>
			if (m_pQuery->GetGenerator() != nullptr)
			{
				SaveGeneratorElement(queryElementToSaveTo->newChild("Generator"));
			}

			// all <InstantEvaluator>s
			for (size_t i = 0; i < m_pQuery->GetInstantEvaluatorCount(); ++i)
			{
				const Core::ITextualInstantEvaluatorBlueprint& textualInstantEvaluatorBP = m_pQuery->GetInstantEvaluator(i);
				XmlNodeRef instantEvaluatorElement = queryElementToSaveTo->newChild("InstantEvaluator");
				SaveInstantEvaluatorElement(instantEvaluatorElement, textualInstantEvaluatorBP);
			}

			// all <DeferredEvaluator>s
			for (size_t i = 0; i < m_pQuery->GetDeferredEvaluatorCount(); ++i)
			{
				const Core::ITextualDeferredEvaluatorBlueprint& textualDeferredEvaluatorBP = m_pQuery->GetDeferredEvaluator(i);
				XmlNodeRef deferredEvaluatorElement = queryElementToSaveTo->newChild("DeferredEvaluator");
				SaveDeferredEvaluatorElement(deferredEvaluatorElement, textualDeferredEvaluatorBP);
			}

			// all child <Query>s
			const Core::ITextualQueryBlueprint* pParent = m_pQuery;
			for (size_t i = 0; i < pParent->GetChildCount(); ++i)
			{
				const Core::ITextualQueryBlueprint& childQueryBP = pParent->GetChild(i);
				m_pQuery = &childQueryBP;
				XmlNodeRef childQueryElement = queryElementToSaveTo->newChild("Query");
				SaveQueryElement(childQueryElement);
			}
			m_pQuery = pParent;
		}

		void CQueryBlueprintSaver_XML::SaveGlobalParamsElement(const XmlNodeRef& globalParamsElementToSaveTo)
		{
			assert(globalParamsElementToSaveTo->isTag("GlobalParams"));

			// <ConstantParam>s
			{
				const Core::ITextualGlobalConstantParamsBlueprint& constantParamsBP = m_pQuery->GetGlobalConstantParams();
				for (size_t i = 0; i < constantParamsBP.GetParameterCount(); ++i)
				{
					const Core::ITextualGlobalConstantParamsBlueprint::SParameterInfo pi = constantParamsBP.GetParameter(i);
					XmlNodeRef constantParamElement = globalParamsElementToSaveTo->newChild("ConstantParam");
					constantParamElement->setAttr("name", pi.szName);
					constantParamElement->setAttr("type", pi.szType);
					constantParamElement->setAttr("value", pi.szValue);
					constantParamElement->setAttr("addToDebugRenderWorld", pi.bAddToDebugRenderWorld);
				}
			}

			// <RuntimeParam>s
			{
				const Core::ITextualGlobalRuntimeParamsBlueprint& runtimeParamsBP = m_pQuery->GetGlobalRuntimeParams();
				for (size_t i = 0; i < runtimeParamsBP.GetParameterCount(); ++i)
				{
					const Core::ITextualGlobalRuntimeParamsBlueprint::SParameterInfo pi = runtimeParamsBP.GetParameter(i);
					XmlNodeRef runtimeParamElement = globalParamsElementToSaveTo->newChild("RuntimeParam");
					runtimeParamElement->setAttr("name", pi.szName);
					runtimeParamElement->setAttr("type", pi.szType);
					runtimeParamElement->setAttr("addToDebugRenderWorld", pi.bAddToDebugRenderWorld);
				}
			}
		}

		void CQueryBlueprintSaver_XML::SaveGeneratorElement(const XmlNodeRef& generatorElementToSaveTo)
		{
			assert(generatorElementToSaveTo->isTag("Generator"));
			assert(m_pQuery->GetGenerator() != nullptr);

			const Core::ITextualGeneratorBlueprint* pGeneratorBP = m_pQuery->GetGenerator();

			// "name"
			generatorElementToSaveTo->setAttr("name", pGeneratorBP->GetGeneratorName());

			// all <Input>s
			{
				const Core::ITextualInputBlueprint& inputRootBP = pGeneratorBP->GetInputRoot();
				for (size_t i = 0; i < inputRootBP.GetChildCount(); ++i)
				{
					const Core::ITextualInputBlueprint& inputBP = inputRootBP.GetChild(i);
					XmlNodeRef inputElement = generatorElementToSaveTo->newChild("Input");
					SaveInputElement(inputElement, inputBP);
				}
			}
		}

		void CQueryBlueprintSaver_XML::SaveInstantEvaluatorElement(const XmlNodeRef& instantEvaluatorElementToSaveTo, const Core::ITextualInstantEvaluatorBlueprint& instantEvaluatorBP)
		{
			assert(instantEvaluatorElementToSaveTo->isTag("InstantEvaluator"));

			// "name" attribute
			instantEvaluatorElementToSaveTo->setAttr("name", instantEvaluatorBP.GetEvaluatorName());

			// "weight" attribute
			instantEvaluatorElementToSaveTo->setAttr("weight", instantEvaluatorBP.GetWeight());

			// "scoreTransform" attribute
			instantEvaluatorElementToSaveTo->setAttr("scoreTransform", instantEvaluatorBP.GetScoreTransform());

			// "negateDiscard" attribute
			instantEvaluatorElementToSaveTo->setAttr("negateDiscard", instantEvaluatorBP.GetNegateDiscard());

			// all <Input>s
			{
				const Core::ITextualInputBlueprint& inputRootBP = instantEvaluatorBP.GetInputRoot();
				for (size_t i = 0; i < inputRootBP.GetChildCount(); ++i)
				{
					const Core::ITextualInputBlueprint& inputBP = inputRootBP.GetChild(i);
					XmlNodeRef inputElement = instantEvaluatorElementToSaveTo->newChild("Input");
					SaveInputElement(inputElement, inputBP);
				}
			}
		}

		void CQueryBlueprintSaver_XML::SaveDeferredEvaluatorElement(const XmlNodeRef& deferredEvaluatorElementToSaveTo, const Core::ITextualDeferredEvaluatorBlueprint& deferredEvaluatorBP)
		{
			assert(deferredEvaluatorElementToSaveTo->isTag("DeferredEvaluator"));

			// "name" attribute
			deferredEvaluatorElementToSaveTo->setAttr("name", deferredEvaluatorBP.GetEvaluatorName());

			// "weight" attribute
			deferredEvaluatorElementToSaveTo->setAttr("weight", deferredEvaluatorBP.GetWeight());

			// "scoreTransform" attribute
			deferredEvaluatorElementToSaveTo->setAttr("scoreTransform", deferredEvaluatorBP.GetScoreTransform());

			// "negateDiscard" attribute
			deferredEvaluatorElementToSaveTo->setAttr("negateDiscard", deferredEvaluatorBP.GetNegateDiscard());

			// all <Input>s
			{
				const Core::ITextualInputBlueprint& inputRootBP = deferredEvaluatorBP.GetInputRoot();
				for (size_t i = 0; i < inputRootBP.GetChildCount(); ++i)
				{
					const Core::ITextualInputBlueprint& inputBP = inputRootBP.GetChild(i);
					XmlNodeRef inputElement = deferredEvaluatorElementToSaveTo->newChild("Input");
					SaveInputElement(inputElement, inputBP);
				}
			}
		}

		void CQueryBlueprintSaver_XML::SaveFunctionElement(const XmlNodeRef& functionElementToSaveTo, const Core::ITextualInputBlueprint& parentInput)
		{
			assert(functionElementToSaveTo->isTag("Function"));

			// <Function>'s "name" attribute (name of the function)
			functionElementToSaveTo->setAttr("name", parentInput.GetFuncName());

			// <Function>'s "addReturnValueToDebugRenderWorldUponExecution" attribute
			functionElementToSaveTo->setAttr("addReturnValueToDebugRenderWorldUponExecution", parentInput.GetAddReturnValueToDebugRenderWorldUponExecution());  // notice: setAttr() will convert the passed in bool to an int

			// <Function>'s "returnValue" attribute (only if we are a leaf-function!)
			if (parentInput.GetChildCount() == 0)
			{
				functionElementToSaveTo->setAttr("returnValue", parentInput.GetFuncReturnValueLiteral());
			}
			else
			{
				// all <Input>s (we're a non-leaf function!)
				for (size_t i = 0; i < parentInput.GetChildCount(); ++i)
				{
					const Core::ITextualInputBlueprint& inputBP = parentInput.GetChild(i);
					XmlNodeRef inputElement = functionElementToSaveTo->newChild("Input");
					SaveInputElement(inputElement, inputBP);
				}
			}
		}

		void CQueryBlueprintSaver_XML::SaveInputElement(const XmlNodeRef& inputElementToSaveTo, const Core::ITextualInputBlueprint& inputBP)
		{
			assert(inputElementToSaveTo->isTag("Input"));

			// "name" attribute (name of the parameter)
			inputElementToSaveTo->setAttr("name", inputBP.GetParamName());

			// <Function> (value of the parameter)
			XmlNodeRef functionElement = inputElementToSaveTo->newChild("Function");

			SaveFunctionElement(functionElement, inputBP);
		}

	}
}
