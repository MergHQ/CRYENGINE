// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ComponentsDictionaryModel.h"

#include "ScriptBrowserUtils.h"

#include <Schematyc/Reflection/Reflection.h>

#include <Schematyc/Script/IScriptView.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/Elements/IScriptComponentInstance.h>

#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/IEnvElement.h>
#include <Schematyc/Env/Elements/IEnvComponent.h>

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

CComponentsDictionary::CComponentsDictionary(const Schematyc::IScriptElement* pScriptScope)
{
	if (pScriptScope)
		Load(pScriptScope);
}

CComponentsDictionary::~CComponentsDictionary()
{

}

const CAbstractDictionaryEntry* CComponentsDictionary::GetEntry(int32 index) const
{
	if (index < m_components.size())
	{
		return &m_components[index];
	}

	return nullptr;
}

QString CComponentsDictionary::GetColumnName(int32 index) const
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

void CComponentsDictionary::Load(const Schematyc::IScriptElement* pScriptScope)
{
	if (pScriptScope)
	{
		m_components.reserve(100);

		bool bAttach = false;

		const Schematyc::IScriptComponentInstance* pScriptComponentInstance = Schematyc::DynamicCast<Schematyc::IScriptComponentInstance>(pScriptScope);
		if (pScriptComponentInstance)
		{
			Schematyc::IEnvRegistry& registry = GetSchematycCore().GetEnvRegistry();
			const Schematyc::IEnvComponent* pEnvComponent = registry.GetComponent(pScriptComponentInstance->GetTypeGUID());
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

		Schematyc::IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptScope->GetGUID());

		VectorSet<Schematyc::SGUID> singletonExclusions;
		auto visitScriptComponentInstance = [this, &singletonExclusions](const Schematyc::IScriptComponentInstance& scriptComponentInstance) -> Schematyc::EVisitStatus
		{
			singletonExclusions.insert(scriptComponentInstance.GetTypeGUID());
			return Schematyc::EVisitStatus::Continue;
		};
		pScriptView->VisitScriptComponentInstances(Schematyc::ScriptComponentInstanceConstVisitor::FromLambda(visitScriptComponentInstance), Schematyc::EDomainScope::Derived);

		auto visitEnvComponentFactory = [this, bAttach, &singletonExclusions, &pScriptView](const Schematyc::IEnvComponent& envComponent) -> Schematyc::EVisitStatus
		{
			const Schematyc::EnvComponentFlags envComponentFlags = envComponent.GetFlags();
			if (!bAttach || envComponentFlags.Check(Schematyc::EEnvComponentFlags::Attach))
			{
				const Schematyc::SGUID envComponentGUID = envComponent.GetGUID();
				if (!envComponentFlags.Check(Schematyc::EEnvComponentFlags::Singleton) || (singletonExclusions.find(envComponentGUID) == singletonExclusions.end()))
				{
					Schematyc::CStackString fullName;
					pScriptView->QualifyName(envComponent, fullName);

					CComponentDictionaryEntry entry;
					entry.m_identifier = envComponentGUID;
					entry.m_name = envComponent.GetName();
					entry.m_fullName = fullName.c_str();
					entry.m_description = envComponent.GetDescription();
					entry.m_wikiLink = envComponent.GetWikiLink();

					m_components.emplace_back(entry);
				}
			}
			return Schematyc::EVisitStatus::Continue;
		};
		pScriptView->VisitEnvComponents(Schematyc::EnvComponentConstVisitor::FromLambda(visitEnvComponentFactory));
	}
}

}
