// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_ComponentsDictionaryModel.h"

#include "Schematyc_ScriptBrowserUtils.h"

#include <Schematyc/Reflection/Schematyc_Reflection.h>

#include <Schematyc/Script/Schematyc_IScriptView.h>
#include <Schematyc/Script/Schematyc_IScriptRegistry.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptComponentInstance.h>

#include <Schematyc/Env/Schematyc_IEnvRegistry.h>
#include <Schematyc/Env/Schematyc_IEnvElement.h>
#include <Schematyc/Env/Elements/Schematyc_IEnvComponent.h>

namespace CrySchematycEditor {

CComponentDictionaryEntry::CComponentDictionaryEntry()
{

}

CComponentDictionaryEntry::~CComponentDictionaryEntry()
{

}

QVariant CComponentDictionaryEntry::GetColumnValue(int32 columnIndex) const
{
	switch (columnIndex)
	{
	case CComponentsDictionary::Column_Type:
		return QVariant::fromValue(m_icon);
	case CComponentsDictionary::Column_Name:
		return QVariant::fromValue(m_name);
	default:
		break;
	}

	return QVariant();
}

QString CComponentDictionaryEntry::GetToolTip() const
{
	return m_description;
}

CComponentsDictionary::CComponentsDictionary()
{

}

CComponentsDictionary::~CComponentsDictionary()
{

}

::CAbstractDictionaryEntry* CComponentsDictionary::GetEntry(int32 index)
{
	if (index < m_components.size())
	{
		return static_cast<::CAbstractDictionaryEntry*>(&m_components[index]);
	}

	return nullptr;
}

QString CComponentsDictionary::GetColumnName(int32 index) const
{
	switch (index)
	{
	case Column_Type:
		return QString("Type");
	case Column_Name:
		return QString("Name");
	default:
		break;
	}

	return QString();
}

void CComponentsDictionary::Load(const Schematyc::IScriptElement* pScriptScope)
{
	if (pScriptScope)
	{
		m_components.reserve(100);

		bool bAttach = false;

		const Schematyc::IScriptComponentInstance* pScriptComponentInstance = Schematyc::DynamicCast<Schematyc::IScriptComponentInstance>(pScriptScope);
		if (pScriptComponentInstance)
		{
			Schematyc::IEnvRegistry& registry = GetSchematycFramework().GetEnvRegistry();
			const Schematyc::IEnvComponent* pEnvComponent = registry.GetComponent(pScriptComponentInstance->GetComponentTypeGUID());
			if (pEnvComponent)
			{
				if (pEnvComponent->GetFlags().Check(Schematyc::EEnvComponentFlags::Socket))
				{
					bAttach = true;
				}
				else
				{
					return;
				}
			}
		}

		Schematyc::IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptScope->GetGUID());
		auto visitEnvComponentFactory = [this, bAttach, &pScriptView](const Schematyc::IEnvComponent& envComponent) -> Schematyc::EVisitStatus
		{
			if (!bAttach || envComponent.GetFlags().Check(Schematyc::EEnvComponentFlags::Attach))
			{
				Schematyc::CStackString fullName;
				pScriptView->QualifyName(envComponent, fullName);

				CComponentDictionaryEntry entry;
				entry.m_identifier = envComponent.GetGUID();
				entry.m_name = envComponent.GetName();
				entry.m_fullName = fullName.c_str();
				entry.m_description = envComponent.GetDescription();
				entry.m_wikiLink = envComponent.GetWikiLink();

				m_components.emplace_back(entry);
			}
			return Schematyc::EVisitStatus::Continue;
		};
		pScriptView->VisitEnvComponents(Schematyc::EnvComponentConstVisitor::FromLambda(visitEnvComponentFactory));
	}
}

}
