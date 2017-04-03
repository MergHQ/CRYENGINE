// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "QueryBlueprintLoader_XML.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace DataSource_XML
	{

		static void DeleteErrorProvider(DataSource::ISyntaxErrorCollector* pSyntaxErrorCollectorToDelete)
		{
			delete pSyntaxErrorCollectorToDelete;
		}

		static DataSource::SyntaxErrorCollectorUniquePtr MakeNewSyntaxErrorCollectorUniquePtr(int lineNumber, const std::shared_ptr<CXMLDataErrorCollector>& pDataErrorCollector)
		{
			return DataSource::SyntaxErrorCollectorUniquePtr(new CSyntaxErrorCollector_XML(lineNumber, pDataErrorCollector), DataSource::CSyntaxErrorCollectorDeleter(&DeleteErrorProvider));
		}

		CQueryBlueprintLoader_XML::CQueryBlueprintLoader_XML(const char* szQueryName, const char* szXmlFilePath, const std::shared_ptr<CXMLDataErrorCollector>& pDataErrorCollector)
			: m_queryName(szQueryName)
			, m_xmlFilePath(szXmlFilePath)
			, m_queryElement()
			, m_pDataErrorCollector(pDataErrorCollector)
			, m_pQuery(nullptr)
		{
			// nothing
		}

		bool CQueryBlueprintLoader_XML::LoadTextualQueryBlueprint(Core::ITextualQueryBlueprint& out, Shared::IUqsString& error)
		{
			// load the XML file
			m_queryElement = gEnv->pSystem->LoadXmlFromFile(m_xmlFilePath.c_str());
			if (!m_queryElement)
			{
				// file not found or invalid xml
				error.Format("XML file '%s' not found or it contains invalid xml", m_xmlFilePath.c_str());
				return false;
			}

			// expect <Query> root node
			if (!m_queryElement->isTag("Query"))
			{
				error.Format("XML file '%s': root node must be <Query>, but is <%s>", m_xmlFilePath.c_str(), m_queryElement->getTag());
				return false;
			}

			bool bSuccess;

			m_pQuery = &out;
			m_pQuery->SetName(m_queryName.c_str());
			bSuccess = ParseQueryElement(m_queryElement, error);
			m_pQuery = nullptr;

			return bSuccess;
		}

		bool CQueryBlueprintLoader_XML::ParseQueryElement(const XmlNodeRef& queryElement, Shared::IUqsString& error)
		{
			assert(queryElement->isTag("Query"));

			m_pQuery->SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(queryElement->getLine(), m_pDataErrorCollector));

			// notice: the name of the query blueprint came in from somewhere outside (e. g. file name) and has already been set by LoadTextualQueryBlueprint()

			// "factory" attribute
			const char* szFactory = queryElement->getAttr("factory");
			m_pQuery->SetQueryFactoryName(szFactory);

			// "maxItemsToKeepInResultSet" attribute
			unsigned int maxItemsToKeepInResultSet = 0;  // notice: getAttrib() doesn't support size_t, so we use unsigned int
			queryElement->getAttr("maxItemsToKeepInResultSet", maxItemsToKeepInResultSet);
			m_pQuery->SetMaxItemsToKeepInResultSet((size_t)maxItemsToKeepInResultSet);

			for (int i = 0; i < queryElement->getChildCount(); ++i)
			{
				XmlNodeRef child = queryElement->getChild(i);

				if (child->isTag("GlobalParams"))
				{
					if (!ParseGlobalParamsElement(child, error))
						return false;
				}
				else if (child->isTag("Generator"))
				{
					if (!ParseGeneratorElement(child, error))
						return false;
				}
				else if (child->isTag("InstantEvaluator"))
				{
					if (!ParseInstantEvaluatorElement(child, error))
						return false;
				}
				else if (child->isTag("DeferredEvaluator"))
				{
					if (!ParseDeferredEvaluatorElement(child, error))
						return false;
				}
				else if (child->isTag("Query"))
				{
					// child queries

					Core::ITextualQueryBlueprint* pParentQuery = m_pQuery;
					m_pQuery = &m_pQuery->AddChild();
					const bool bSucceeded = ParseQueryElement(child, error);
					m_pQuery = pParentQuery;

					if (!bSucceeded)
						return false;
				}
				// TODO: <SecondaryGenerator> element(s) ?
				else
				{
					error.Format("line #%i: unknown tag: <%s>", child->getLine(), child->getTag());
					return false;
				}
			}

			return true;
		}

		bool CQueryBlueprintLoader_XML::ParseGlobalParamsElement(const XmlNodeRef& globalParamsElement, Shared::IUqsString& error)
		{
			assert(globalParamsElement->isTag("GlobalParams"));

			// <ConstantParam> and <RuntimeParam> elements
			for (int i = 0; i < globalParamsElement->getChildCount(); ++i)
			{
				XmlNodeRef child = globalParamsElement->getChild(i);

				if (child->isTag("ConstantParam"))
				{
					// "name" attribute
					const char* szName;
					if (!child->getAttr("name", &szName))
					{
						error.Format("line #%i: <ConstantParam> is missing the attribute 'name'", child->getLine());
						return false;
					}

					// "type" attribute
					const char* szType;
					if (!child->getAttr("type", &szType))
					{
						error.Format("line #%i: <ConstantParam> is missing the attribute 'type'", child->getLine());
						return false;
					}

					// "value" attribute
					const char* szValue;
					if (!child->getAttr("value", &szValue))
					{
						error.Format("line #%i: <ConstantParam> is missing the attribute 'value'", child->getLine());
						return false;
					}

					// "addToDebugRenderWorld" attribute (optional because it got introduced after some query blueprints were already in use; default to 'false')
					bool bAddToDebugRenderWorld = false;
					child->getAttr("addToDebugRenderWorld", bAddToDebugRenderWorld);

					m_pQuery->GetGlobalConstantParams().AddParameter(szName, szType, szValue, bAddToDebugRenderWorld, MakeNewSyntaxErrorCollectorUniquePtr(child->getLine(), m_pDataErrorCollector));
				}
				else if (child->isTag("RuntimeParam"))
				{
					// "name" attribute
					const char* szName;
					if (!child->getAttr("name", &szName))
					{
						error.Format("line #%i: <RuntimeParam> is missing the attribute 'name'", child->getLine());
						return false;
					}

					// "type" attribute
					const char* szType;
					if (!child->getAttr("type", &szType))
					{
						error.Format("line #%i: <RuntimeParam> is missing the attribute 'type'", child->getLine());
						return false;
					}

					// "addToDebugRenderWorld" attribute (optional because it got introduced after some query blueprints were already in use; default to 'false')
					bool bAddToDebugRenderWorld = false;
					child->getAttr("addToDebugRenderWorld", bAddToDebugRenderWorld);

					m_pQuery->GetGlobalRuntimeParams().AddParameter(szName, szType, bAddToDebugRenderWorld, MakeNewSyntaxErrorCollectorUniquePtr(child->getLine(), m_pDataErrorCollector));
				}
				else
				{
					error.Format("line #%i: unknown tag: <%s>", child->getLine(), child->getTag());
					return false;
				}
			}

			return true;
		}

		bool CQueryBlueprintLoader_XML::ParseGeneratorElement(const XmlNodeRef& generatorElement, Shared::IUqsString& error)
		{
			assert(generatorElement->isTag("Generator"));

			Core::ITextualGeneratorBlueprint& textualGeneratorBP = m_pQuery->SetGenerator();
			textualGeneratorBP.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(generatorElement->getLine(), m_pDataErrorCollector));

			// "name" attribute
			const char* szName;
			if (!generatorElement->getAttr("name", &szName))
			{
				error.Format("line #%i: <Generator> is missing the attribute 'name'", generatorElement->getLine());
				return false;
			}
			textualGeneratorBP.SetGeneratorName(szName);

			// parse <Input> elements (expect only these elements)
			Core::ITextualInputBlueprint& textualInputRoot = textualGeneratorBP.GetInputRoot();
			textualInputRoot.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(generatorElement->getLine(), m_pDataErrorCollector));	// same as its parent (the textual-generator-blueprint)
			for (int i = 0; i < generatorElement->getChildCount(); ++i)
			{
				XmlNodeRef child = generatorElement->getChild(i);

				if (child->isTag("Input"))
				{
					if (!ParseInputElement(child, textualInputRoot, error))
						return false;
				}
				else
				{
					error.Format("line #%i: unknown tag: <%s>", child->getLine(), child->getTag());
					return false;
				}
			}

			return true;
		}

		bool CQueryBlueprintLoader_XML::ParseInstantEvaluatorElement(const XmlNodeRef& instantEvaluatorElement, Shared::IUqsString& error)
		{
			assert(instantEvaluatorElement->isTag("InstantEvaluator"));

			Core::ITextualInstantEvaluatorBlueprint& textualInstantEvaluatorBP = m_pQuery->AddInstantEvaluator();
			textualInstantEvaluatorBP.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(instantEvaluatorElement->getLine(), m_pDataErrorCollector));

			// "name" attribute
			const char* szEvaluatorName;
			if (!instantEvaluatorElement->getAttr("name", &szEvaluatorName))
			{
				error.Format("line #%i: <InstantEvaluator> is missing the attribute 'name'", instantEvaluatorElement->getLine());
				return false;
			}
			textualInstantEvaluatorBP.SetEvaluatorName(szEvaluatorName);

			// "weight" attribute
			float weight = 1.0f;
			if (!instantEvaluatorElement->getAttr("weight", weight))
			{
				error.Format("line #%i: <InstantEvaluator> is missing the attribute 'weight'", instantEvaluatorElement->getLine());
				return false;
			}
			textualInstantEvaluatorBP.SetWeight(weight);

			// "scoreTransform" attribute (treat it as optional for older queries, so that the default score-transform will get picked in such a case)
			const char* szScoreTransform = "";
			if (instantEvaluatorElement->getAttr("scoreTransform", &szScoreTransform))
			{
				textualInstantEvaluatorBP.SetScoreTransform(szScoreTransform);
			}

			// "negateDiscard" attribute
			bool bNegateDiscard = false;
			instantEvaluatorElement->getAttr("negateDiscard", bNegateDiscard);
			textualInstantEvaluatorBP.SetNegateDiscard(bNegateDiscard);

			// parse <Input> elements (expect only these elements)
			Core::ITextualInputBlueprint& textualInputRoot = textualInstantEvaluatorBP.GetInputRoot();
			textualInputRoot.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(instantEvaluatorElement->getLine(), m_pDataErrorCollector));	// same as its parent (the textual-instant-evaluator-blueprint)
			for (int i = 0; i < instantEvaluatorElement->getChildCount(); ++i)
			{
				XmlNodeRef child = instantEvaluatorElement->getChild(i);

				if (child->isTag("Input"))
				{
					if (!ParseInputElement(child, textualInputRoot, error))
						return false;
				}
				else
				{
					error.Format("line #%i: unknown tag: <%s>", child->getLine(), child->getTag());
					return false;
				}
			}

			return true;
		}

		bool CQueryBlueprintLoader_XML::ParseDeferredEvaluatorElement(const XmlNodeRef& deferredEvaluatorElement, Shared::IUqsString& error)
		{
			assert(deferredEvaluatorElement->isTag("DeferredEvaluator"));

			Core::ITextualDeferredEvaluatorBlueprint& textualDeferredEvaluatorBP = m_pQuery->AddDeferredEvaluator();
			textualDeferredEvaluatorBP.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(deferredEvaluatorElement->getLine(), m_pDataErrorCollector));

			// "name" attribute
			const char* szEvaluatorName;
			if (!deferredEvaluatorElement->getAttr("name", &szEvaluatorName))
			{
				error.Format("line #%i: <DeferredEvaluator> is missing the attribute 'name'", deferredEvaluatorElement->getLine());
				return false;
			}
			textualDeferredEvaluatorBP.SetEvaluatorName(szEvaluatorName);

			// "weight" attribute
			float weight = 1.0f;
			if (!deferredEvaluatorElement->getAttr("weight", weight))
			{
				error.Format("line #%i: <DeferredEvaluator> is missing the attribute 'weight'", deferredEvaluatorElement->getLine());
				return false;
			}
			textualDeferredEvaluatorBP.SetWeight(weight);

			// "scoreTransform" attribute (treat it as optional for older queries, so that the default score-transform will get picked in such a case)
			const char* szScoreTransform = "";
			if (deferredEvaluatorElement->getAttr("scoreTransform", &szScoreTransform))
			{
				textualDeferredEvaluatorBP.SetScoreTransform(szScoreTransform);
			}

			// "negateDiscard" attribute
			bool bNegateDiscard = false;
			deferredEvaluatorElement->getAttr("negateDiscard", bNegateDiscard);
			textualDeferredEvaluatorBP.SetNegateDiscard(bNegateDiscard);

			// parse <Input> elements (expect only these elements)
			Core::ITextualInputBlueprint& textualInputRoot = textualDeferredEvaluatorBP.GetInputRoot();
			textualInputRoot.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(deferredEvaluatorElement->getLine(), m_pDataErrorCollector));	// same as its parent (the textual-deferred-evaluator-blueprint)
			for (int i = 0; i < deferredEvaluatorElement->getChildCount(); ++i)
			{
				XmlNodeRef child = deferredEvaluatorElement->getChild(i);

				if (child->isTag("Input"))
				{
					if (!ParseInputElement(child, textualInputRoot, error))
						return false;
				}
				else
				{
					error.Format("line #%i: unknown tag: <%s>", child->getLine(), child->getTag());
					return false;
				}
			}

			return true;
		}

		bool CQueryBlueprintLoader_XML::ParseFunctionElement(const XmlNodeRef& functionElement, Core::ITextualInputBlueprint& parentInput, Shared::IUqsString& error)
		{
			assert(functionElement->isTag("Function"));

			// whether the "returnValue" attribute is present specifies whether it's a leaf- or non-leaf function

			if (functionElement->haveAttr("returnValue"))
			{
				// leaf function
				// TODO: do something with that "returnValue" attribute
			}
			else
			{
				// non-leaf function => recurse further down (expect <Input> elements)
				for (int i = 0; i < functionElement->getChildCount(); ++i)
				{
					XmlNodeRef child = functionElement->getChild(i);

					if (child->isTag("Input"))
					{
						if (!ParseInputElement(child, parentInput, error))
							return false;
					}
					else
					{
						error.Format("line #%i: unknown tag: <%s>", child->getLine(), child->getTag());
						return false;
					}
				}
			}

			return true;
		}

		bool CQueryBlueprintLoader_XML::ParseInputElement(const XmlNodeRef& inputElement, Core::ITextualInputBlueprint& parentInput, Shared::IUqsString& error)
		{
			assert(inputElement->isTag("Input"));

			// "name" attribute (name of the parameter)
			const char* szParamName;
			if (!inputElement->getAttr("name", &szParamName))
			{
				error.Format("line #%i: <Input> is missing the attribute 'name'", inputElement->getLine());
				return false;
			}

			// <Function> element
			XmlNodeRef functionElement = inputElement->findChild("Function");
			if (!functionElement)
			{
				error.Format("line #%i: <Input> is missing the <Function> element", inputElement->getLine());
				return false;
			}

			// <Function>'s "name" attribute (name of the function)
			const char* szFunctionName;
			if (!functionElement->getAttr("name", &szFunctionName))
			{
				error.Format("line #%i: <Function> is missing the attribute 'name'", functionElement->getLine());
				return false;
			}

			//  "returnValue"
			const char* szFunctionReturnValue = functionElement->getAttr("returnValue");   // optional attribute; defaults to ""

			// "addReturnValueToDebugRenderWorldUponExecution"
			bool bAddReturnValueToDebugRenderWorldUponExecution = false;
			functionElement->getAttr("addReturnValueToDebugRenderWorldUponExecution", bAddReturnValueToDebugRenderWorldUponExecution);

			Core::ITextualInputBlueprint& newChild = parentInput.AddChild(szParamName, szFunctionName, szFunctionReturnValue, bAddReturnValueToDebugRenderWorldUponExecution);
			newChild.SetSyntaxErrorCollector(MakeNewSyntaxErrorCollectorUniquePtr(functionElement->getLine(), m_pDataErrorCollector));

			return ParseFunctionElement(functionElement, newChild, error);
		}

	}
}
