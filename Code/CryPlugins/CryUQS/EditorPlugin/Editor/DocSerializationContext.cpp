// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "DocSerializationContext.h"
#include "Document.h"
#include "EditorContext.h"

//////////////////////////////////////////////////////////////////////////
// CUqsDocSerializationContext
//////////////////////////////////////////////////////////////////////////

uqs::client::IItemFactory* CUqsDocSerializationContext::GetItemFactoryByName(const SItemTypeName& typeName) const
{
	const auto& db = uqs::core::IHubPlugin::GetHub().GetItemFactoryDatabase();
	return db.FindFactoryByName(typeName.c_str());
}

uqs::client::IGeneratorFactory* CUqsDocSerializationContext::GetGeneratorFactoryByName(const char* szName) const
{
	const auto& db = uqs::core::IHubPlugin::GetHub().GetGeneratorFactoryDatabase();
	return db.FindFactoryByName(szName);
}

uqs::client::IFunctionFactory* CUqsDocSerializationContext::GetFunctionFactoryByName(const char* szName) const
{
	const auto& db = uqs::core::IHubPlugin::GetHub().GetFunctionFactoryDatabase();
	return db.FindFactoryByName(szName);
}

uqs::client::IInstantEvaluatorFactory* CUqsDocSerializationContext::GetInstantEvaluatorFactoryByName(const char* szName) const
{
	const auto& db = uqs::core::IHubPlugin::GetHub().GetInstantEvaluatorFactoryDatabase();
	return db.FindFactoryByName(szName);
}

uqs::client::IDeferredEvaluatorFactory* CUqsDocSerializationContext::GetDeferredEvaluatorFactoryByName(const char* szName) const
{
	const auto& db = uqs::core::IHubPlugin::GetHub().GetDeferredEvaluatorFactoryDatabase();
	return db.FindFactoryByName(szName);
}

uqs::core::IQueryFactory* CUqsDocSerializationContext::GetQueryFactoryByName(const char* szName) const
{
	const auto& db = uqs::core::IHubPlugin::GetHub().GetQueryFactoryDatabase();
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

SItemTypeName CUqsDocSerializationContext::GetItemTypeNameFromType(const uqs::shared::CTypeInfo& typeInfo) const
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

uqseditor::CParametersListContext* CUqsDocSerializationContext::GetParametersListContext() const
{
	if (!m_paramListContextStack.empty())
		return m_paramListContextStack.top();
	return nullptr;
}

uqseditor::CSelectedGeneratorContext* CUqsDocSerializationContext::GetSelectedGeneratorContext() const
{
	if (!m_selectedGeneratorContextStack.empty())
		return m_selectedGeneratorContextStack.top();
	return nullptr;
}

void CUqsDocSerializationContext::PushParametersListContext(uqseditor::CParametersListContext* pContext)
{
	m_paramListContextStack.push(pContext);
}

void CUqsDocSerializationContext::PopParametersListContext(uqseditor::CParametersListContext* pContext)
{
	CRY_ASSERT(!m_paramListContextStack.empty() && m_paramListContextStack.top() == pContext);
	m_paramListContextStack.pop();
}

void CUqsDocSerializationContext::PushSelectedGeneratorContext(uqseditor::CSelectedGeneratorContext* pContext)
{
	m_selectedGeneratorContextStack.push(pContext);
}

void CUqsDocSerializationContext::PopSelectedGeneratorContext(uqseditor::CSelectedGeneratorContext* pContext)
{
	CRY_ASSERT(!m_selectedGeneratorContextStack.empty() && m_selectedGeneratorContextStack.top() == pContext);
	m_selectedGeneratorContextStack.pop();
}
