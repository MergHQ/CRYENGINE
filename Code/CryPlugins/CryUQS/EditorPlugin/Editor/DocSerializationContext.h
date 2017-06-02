// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include <CrySerialization/StringList.h>

class CUqsQueryDocument;
class CUqsEditorContext;
struct SDocumentSettings;
struct SItemTypeName;
class CUqsDatabaseSerializationCache;

namespace UQSEditor
{
class CParametersListContext;
class CSelectedGeneratorContext;
} // namespace UQSEditor

class CUqsDocSerializationContext
{
public:
	explicit CUqsDocSerializationContext(const CUqsEditorContext& editorContext)
		: m_editorContext(editorContext)
		, m_bForceSelectionHelpersEvaluation(false)
		, m_paramListContextStack()
		, m_selectedGeneratorContextStack()
	{}

	UQS::Client::IItemFactory*              GetItemFactoryByName(const SItemTypeName& typeName) const;
	UQS::Client::IGeneratorFactory*         GetGeneratorFactoryByName(const char* szName) const;
	UQS::Client::IFunctionFactory*          GetFunctionFactoryByName(const char* szName) const;
	UQS::Client::IInstantEvaluatorFactory*  GetInstantEvaluatorFactoryByName(const char* szName) const;
	UQS::Client::IDeferredEvaluatorFactory* GetDeferredEvaluatorFactoryByName(const char* szName) const;
	UQS::Core::IQueryFactory*               GetQueryFactoryByName(const char* szName) const;
	UQS::Core::IScoreTransformFactory*      GetScoreTransformFactoryByName(const char* szName) const;

	const Serialization::StringList&        GetQueryFactoryNamesList() const;
	const Serialization::StringList&        GetItemTypeNamesList() const;
	const Serialization::StringList&        GetGeneratorNamesList() const;
	const Serialization::StringList&        GetFunctionNamesList(const SItemTypeName& typeToFilter) const;
	const Serialization::StringList&        GetEvaluatorNamesList() const;
	const Serialization::StringList&        GetScoreTransformNamesList() const;

	SItemTypeName                           GetItemTypeNameFromType(const UQS::Shared::CTypeInfo& typeInfo) const;

	const SDocumentSettings&              GetSettings() const;

	bool                                  ShouldForceSelectionHelpersEvaluation() const;
	void                                  SetShouldForceSelectionHelpersEvaluation(const bool value) { m_bForceSelectionHelpersEvaluation = value; }

	UQSEditor::CParametersListContext*    GetParametersListContext() const;
	UQSEditor::CSelectedGeneratorContext* GetSelectedGeneratorContext() const;

private:
	friend class UQSEditor::CParametersListContext;
	void PushParametersListContext(UQSEditor::CParametersListContext* pContext);
	void PopParametersListContext(UQSEditor::CParametersListContext* pContext);

	friend class UQSEditor::CSelectedGeneratorContext;
	void PushSelectedGeneratorContext(UQSEditor::CSelectedGeneratorContext* pContext);
	void PopSelectedGeneratorContext(UQSEditor::CSelectedGeneratorContext* pContext);

private:
	const CUqsEditorContext&                          m_editorContext;
	bool                                              m_bForceSelectionHelpersEvaluation;
	std::stack<UQSEditor::CParametersListContext*>    m_paramListContextStack;
	std::stack<UQSEditor::CSelectedGeneratorContext*> m_selectedGeneratorContextStack;
};
