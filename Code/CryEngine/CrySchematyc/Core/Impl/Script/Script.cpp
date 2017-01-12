// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Script.h"

#include <CrySystem/File/ICryPak.h>
#include <CrySystem/ITimer.h>
#include <Schematyc/ICore.h>
#include <Schematyc/Script/IScriptElement.h>
#include <Schematyc/SerializationUtils/SerializationToString.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/Utils/StringUtils.h>

namespace Schematyc
{
CScript::CScript(const SGUID& guid, const char* szName)
	: m_guid(guid)
	, m_name(szName)
	, m_timeStamp(gEnv->pTimer->GetAsyncTime())
	, m_pRoot(nullptr)
{}

SGUID CScript::GetGUID() const
{
	return m_guid;
}

void CScript::SetName(const char* szName)
{
	m_name = szName;
}

const char* CScript::SetNameFromRoot()
{
	// #SchematycTODO : Only store names relative to script folder and re-construct path when saving?

	if (m_pRoot)
	{
		CStackString name;

		SetNameFromRootRecursive(name, *m_pRoot);

		StringUtils::ToSnakeCase(name);

		string path = gEnv->pCryPak->GetGameFolder();
		path.append("/");
		path.append(gEnv->pSchematyc->GetScriptsFolder());
		path.append("/");

		name.insert(0, path);

		switch (m_pRoot->GetType())
		{
		case EScriptElementType::Class:
			{
				name.append(".sc_class");
				break;
			}
		default:
			{
				name.append(".sc_lib");
				break;
			}
		}

		name.MakeLower();

		m_name = name.c_str();
	}

	return m_name.c_str();
}

const char* CScript::GetName() const
{
	return m_name;
}

const CTimeValue& CScript::GetTimeStamp() const
{
	return m_timeStamp;
}

void CScript::SetRoot(IScriptElement* pRoot)
{
	m_pRoot = pRoot;
}

IScriptElement* CScript::GetRoot()
{
	return m_pRoot;
}

EVisitStatus CScript::VisitElements(const ScriptElementVisitor& visitor)
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (m_pRoot && !visitor.IsEmpty())
	{
		return VisitElementsRecursive(visitor, *m_pRoot);
	}
	return EVisitStatus::Stop;
}

EVisitStatus CScript::VisitElementsRecursive(const ScriptElementVisitor& visitor, IScriptElement& element)
{
	EVisitStatus visitStatus = visitor(element);
	if (visitStatus == EVisitStatus::Continue)
	{
		for (IScriptElement* pChild = element.GetFirstChild(); pChild; pChild = pChild->GetNextSibling())
		{
			if (!pChild->GetScript())
			{
				visitStatus = VisitElementsRecursive(visitor, *pChild);
				if (visitStatus == EVisitStatus::Stop)
				{
					break;
				}
			}
		}
	}
	return visitStatus;
}

void CScript::SetNameFromRootRecursive(CStackString& name, IScriptElement& element)
{
	IScriptElement* pParent = element.GetParent();
	if (pParent && (pParent->GetType() != EScriptElementType::Root))
	{
		SetNameFromRootRecursive(name, *pParent);
		name.append("/");
	}
	name.append(element.GetName());
}
}
