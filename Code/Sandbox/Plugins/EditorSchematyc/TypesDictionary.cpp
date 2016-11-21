// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TypesDictionary.h"

#include "ScriptBrowserUtils.h"

#include <Schematyc/Reflection/Reflection.h>

#include <Schematyc/Script/IScriptView.h>
#include <Schematyc/Script/IScriptRegistry.h>

#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/IEnvElement.h>
#include <Schematyc/Env/Elements/IEnvDataType.h>

namespace CrySchematycEditor {

CTypeDictionaryEntry::CTypeDictionaryEntry()
{

}

CTypeDictionaryEntry::~CTypeDictionaryEntry()
{

}

QVariant CTypeDictionaryEntry::GetColumnValue(int32 columnIndex) const
{
	switch (columnIndex)
	{
	case CTypesDictionary::Column_Name:
		return QVariant::fromValue(m_name);
	default:
		break;
	}

	return QVariant();
}

CTypesDictionary::CTypesDictionary(const Schematyc::IScriptElement* pScriptScope)
{
	Load(pScriptScope);
}

CTypesDictionary::~CTypesDictionary()
{

}

const CAbstractDictionaryEntry* CTypesDictionary::GetEntry(int32 index) const
{
	if (index < m_types.size())
	{
		return static_cast<const CAbstractDictionaryEntry*>(&m_types[index]);
	}

	return nullptr;
}

QString CTypesDictionary::GetColumnName(int32 index) const
{
	switch (index)
	{
	case Column_Name:
		return QString("Name");
	default:
		break;
	}

	return QString();
}

void CTypesDictionary::Load(const Schematyc::IScriptElement* pScriptScope)
{
	if (pScriptScope)
	{
		m_types.reserve(50);

		Schematyc::IScriptViewPtr pScriptView = gEnv->pSchematyc->CreateScriptView(pScriptScope->GetGUID());

		auto visitEnvType = [this, &pScriptView](const Schematyc::IEnvDataType& envType) -> Schematyc::EVisitStatus
		{
			Schematyc::CStackString name;
			pScriptView->QualifyName(envType, name);

			CTypeDictionaryEntry entry;
			entry.m_name = name.c_str();
			entry.m_elementId = Schematyc::SElementId(Schematyc::EDomain::Env, envType.GetGUID());
			m_types.push_back(entry);

			return Schematyc::EVisitStatus::Continue;
		};
		pScriptView->VisitEnvDataTypes(Schematyc::EnvDataTypeConstVisitor::FromLambda(visitEnvType));

		auto visitScriptEnum = [this, &pScriptView](const Schematyc::IScriptEnum& scriptEnum)
		{
			Schematyc::CStackString name;
			pScriptView->QualifyName(scriptEnum, Schematyc::EDomainQualifier::Global, name);

			CTypeDictionaryEntry entry;
			entry.m_name = name.c_str();
			entry.m_elementId = Schematyc::SElementId(Schematyc::EDomain::Script, scriptEnum.GetGUID());
			m_types.push_back(entry);
		};
		pScriptView->VisitAccesibleEnums(Schematyc::ScriptEnumConstVisitor::FromLambda(visitScriptEnum));

		auto visitScriptStruct = [this, &pScriptView](const Schematyc::IScriptStruct& scriptStruct)
		{
			Schematyc::CStackString name;
			pScriptView->QualifyName(scriptStruct, Schematyc::EDomainQualifier::Global, name);

			CTypeDictionaryEntry entry;
			entry.m_name = name.c_str();
			entry.m_elementId = Schematyc::SElementId(Schematyc::EDomain::Script, scriptStruct.GetGUID());
			m_types.push_back(entry);

			return Schematyc::EVisitStatus::Continue;
		};
		//pScriptView->VisitScriptStructs(ScriptEnumConstVisitor::FromLambda(visitScriptStruct), EDomainScope::Local);
	}
}

}
