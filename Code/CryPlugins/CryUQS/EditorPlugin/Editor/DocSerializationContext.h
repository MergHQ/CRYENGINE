// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include <CrySerialization/StringList.h>

class CUqsQueryDocument;
class CUqsEditorContext;
struct SDocumentSettings;
struct SItemTypeName;
class CUqsDatabaseSerializationCache;

namespace uqseditor
{
class CParametersListContext;
class CSelectedGeneratorContext;
} // namespace uqseditor

class CUqsDocSerializationContext
{
public:
	explicit CUqsDocSerializationContext(const CUqsEditorContext& editorContext)
		: m_editorContext(editorContext)
		, m_bForceSelectionHelpersEvaluation(false)
		, m_paramListContextStack()
		, m_selectedGeneratorContextStack()
	{}

	uqs::client::IItemFactory*              GetItemFactoryByName(const SItemTypeName& typeName) const;
	uqs::client::IGeneratorFactory*         GetGeneratorFactoryByName(const char* szName) const;
	uqs::client::IFunctionFactory*          GetFunctionFactoryByName(const char* szName) const;
	uqs::client::IInstantEvaluatorFactory*  GetInstantEvaluatorFactoryByName(const char* szName) const;
	uqs::client::IDeferredEvaluatorFactory* GetDeferredEvaluatorFactoryByName(const char* szName) const;
	uqs::core::IQueryFactory*               GetQueryFactoryByName(const char* szName) const;

	const Serialization::StringList&        GetQueryFactoryNamesList() const;
	const Serialization::StringList&        GetItemTypeNamesList() const;
	const Serialization::StringList&        GetGeneratorNamesList() const;
	const Serialization::StringList&        GetFunctionNamesList(const SItemTypeName& typeToFilter) const;
	const Serialization::StringList&        GetEvaluatorNamesList() const;

	SItemTypeName                           GetItemTypeNameFromType(const uqs::shared::CTypeInfo& typeInfo) const;

	const SDocumentSettings&              GetSettings() const;

	bool                                  ShouldForceSelectionHelpersEvaluation() const;
	void                                  SetShouldForceSelectionHelpersEvaluation(const bool value) { m_bForceSelectionHelpersEvaluation = value; }

	uqseditor::CParametersListContext*    GetParametersListContext() const;
	uqseditor::CSelectedGeneratorContext* GetSelectedGeneratorContext() const;

private:
	friend class uqseditor::CParametersListContext;
	void PushParametersListContext(uqseditor::CParametersListContext* pContext);
	void PopParametersListContext(uqseditor::CParametersListContext* pContext);

	friend class uqseditor::CSelectedGeneratorContext;
	void PushSelectedGeneratorContext(uqseditor::CSelectedGeneratorContext* pContext);
	void PopSelectedGeneratorContext(uqseditor::CSelectedGeneratorContext* pContext);

private:
	const CUqsEditorContext&                          m_editorContext;
	bool                                              m_bForceSelectionHelpersEvaluation;
	std::stack<uqseditor::CParametersListContext*>    m_paramListContextStack;
	std::stack<uqseditor::CSelectedGeneratorContext*> m_selectedGeneratorContextStack;
};
