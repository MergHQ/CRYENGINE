// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "DocSerializationContext.h"
#include "Document.h"
#include "EditorContext.h"

//////////////////////////////////////////////////////////////////////////
// CUqsDocSerializationContext
//////////////////////////////////////////////////////////////////////////

UQS::Client::IItemFactory* CUqsDocSerializationContext::GetItemFactoryByName(const SItemTypeName& typeName) const
{
	const auto& db = UQS::Core::IHubPlugin::GetHub().GetItemFactoryDatabase();
	return db.FindFactoryByName(typeName.c_str());
}

UQS::Client::IGeneratorFactory* CUqsDocSerializationContext::GetGeneratorFactoryByName(const char* szName) const
{
	const auto& db = UQS::Core::IHubPlugin::GetHub().GetGeneratorFactoryDatabase();
	return db.FindFactoryByName(szName);
}

UQS::Client::IFunctionFactory* CUqsDocSerializationContext::GetFunctionFactoryByName(const char* szName) const
{
	const auto& db = UQS::Core::IHubPlugin::GetHub().GetFunctionFactoryDatabase();
	return db.FindFactoryByName(szName);
}

UQS::Client::IInstantEvaluatorFactory* CUqsDocSerializationContext::GetInstantEvaluatorFactoryByName(const char* szName) const
{
	const auto& db = UQS::Core::IHubPlugin::GetHub().GetInstantEvaluatorFactoryDatabase();
	return db.FindFactoryByName(szName);
}

UQS::Client::IDeferredEvaluatorFactory* CUqsDocSerializationContext::GetDeferredEvaluatorFactoryByName(const char* szName) const
{
	const auto& db = UQS::Core::IHubPlugin::GetHub().GetDeferredEvaluatorFactoryDatabase();
	return db.FindFactoryByName(szName);
}

UQS::Core::IQueryFactory* CUqsDocSerializationContext::GetQueryFactoryByName(const char* szName) const
{
	const auto& db = UQS::Core::IHubPlugin::GetHub().GetQueryFactoryDatabase();
	return db.FindFactoryByName(szName);
}

UQS::Core::IScoreTransformFactory* CUqsDocSerializationContext::GetScoreTransformFactoryByName(const char* szName) const
{
	const auto& db = UQS::Core::IHubPlugin::GetHub().GetScoreTransformFactoryDatabase();
	return db.FindFactoryByName(szName);
}

const Serialization::StringList& CUqsDocSerializationContext::GetQueryFactoryNamesList() const
{
	return m_editorContext.GetSerializationCache().GetQueryFactoryNamesList();
}

const Serialization::StringList& CUqsDocSerializationContext::GetItemTypeNamesList() const
{
	return m_editorContext.GetSerializationCache().GetItemTypeNamesList();
}

const Serialization::StringList& CUqsDocSerializationContext::GetGeneratorNamesList() const
{
	return m_editorContext.GetSerializationCache().GetGeneratorNamesList();
}

const Serialization::StringList& CUqsDocSerializationContext::GetFunctionNamesList(const SItemTypeName& typeToFilter) const
{
	if (GetSettings().bFilterAvailableInputsByType)
	{
		return m_editorContext.GetSerializationCache().GetFunctionNamesList(typeToFilter);
	}
	else
	{
		return m_editorContext.GetSerializationCache().GetFunctionNamesList();
	}
}

const Serialization::StringList& CUqsDocSerializationContext::GetEvaluatorNamesList() const
{
	return m_editorContext.GetSerializationCache().GetEvaluatorNamesList();
}

const Serialization::StringList& CUqsDocSerializationContext::GetScoreTransformNamesList() const
{
	return m_editorContext.GetSerializationCache().GetScoreTransformNamesList();
}

SItemTypeName CUqsDocSerializationContext::GetItemTypeNameFromType(const UQS::Shared::CTypeInfo& typeInfo) const
{
	return m_editorContext.GetSerializationCache().GetItemTypeNameFromType(typeInfo);
}

const SDocumentSettings& CUqsDocSerializationContext::GetSettings() const
{
	return m_editorContext.GetSettings();
}

bool CUqsDocSerializationContext::ShouldForceSelectionHelpersEvaluation() const
{
	return m_bForceSelectionHelpersEvaluation;
}

UQSEditor::CParametersListContext* CUqsDocSerializationContext::GetParametersListContext() const
{
	if (!m_paramListContextStack.empty())
		return m_paramListContextStack.top();
	return nullptr;
}

UQSEditor::CSelectedGeneratorContext* CUqsDocSerializationContext::GetSelectedGeneratorContext() const
{
	if (!m_selectedGeneratorContextStack.empty())
		return m_selectedGeneratorContextStack.top();
	return nullptr;
}

void CUqsDocSerializationContext::PushParametersListContext(UQSEditor::CParametersListContext* pContext)
{
	m_paramListContextStack.push(pContext);
}

void CUqsDocSerializationContext::PopParametersListContext(UQSEditor::CParametersListContext* pContext)
{
	CRY_ASSERT(!m_paramListContextStack.empty() && m_paramListContextStack.top() == pContext);
	m_paramListContextStack.pop();
}

void CUqsDocSerializationContext::PushSelectedGeneratorContext(UQSEditor::CSelectedGeneratorContext* pContext)
{
	m_selectedGeneratorContextStack.push(pContext);
}

void CUqsDocSerializationContext::PopSelectedGeneratorContext(UQSEditor::CSelectedGeneratorContext* pContext)
{
	CRY_ASSERT(!m_selectedGeneratorContextStack.empty() && m_selectedGeneratorContextStack.top() == pContext);
	m_selectedGeneratorContextStack.pop();
}
