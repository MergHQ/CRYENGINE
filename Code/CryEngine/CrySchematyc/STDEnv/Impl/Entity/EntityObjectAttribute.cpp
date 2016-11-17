// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObjectAttribute.h"

#include <CrySerialization/Decorators/ActionButton.h>

namespace Schematyc
{
CEntityObjectAttribute::CEntityObjectAttribute(const SGUID& guid, const IObjectProperties& defaultProperties)
	: m_guid(guid)
	, m_pProperties(defaultProperties.Clone())
{}

CEntityObjectAttribute::CEntityObjectAttribute(const CEntityObjectAttribute& rhs)
	: m_guid(rhs.m_guid)
	, m_pProperties(rhs.m_pProperties->Clone())
{}

const char* CEntityObjectAttribute::GetName() const
{
	return ms_szAttributeName;
}

const char* CEntityObjectAttribute::GetLabel() const
{
	return "Schematyc";
}

void CEntityObjectAttribute::Serialize(Serialization::IArchive& archive)
{
	if (archive.isEdit() && archive.isOutput())
	{
		DisplayDetails(archive);
	}

	if (m_pProperties)
	{
		archive(*m_pProperties, "properties", "Properties");

		if (archive.isInput())
		{
			m_signals.change.Send();
		}
	}
}

IEntityAttributePtr CEntityObjectAttribute::Clone() const
{
	return std::make_shared<CEntityObjectAttribute>(*this);
}

SGUID CEntityObjectAttribute::GetGUID() const
{
	return m_guid;
}

IObjectPropertiesConstPtr CEntityObjectAttribute::GetProperties() const
{
	return m_pProperties;
}

EntityPropertiesChangeSignal::Slots& CEntityObjectAttribute::GetChangeSignalSlots()
{
	return m_signals.change.GetSlots();
}

void CEntityObjectAttribute::DisplayDetails(Serialization::IArchive& archive)
{
	if (archive.openBlock("overview", "Overview"))
	{
		const IScriptClass* pScriptClass = DynamicCast<IScriptClass>(GetSchematycCore().GetScriptRegistry().GetElement(m_guid));
		if (pScriptClass)
		{
			{
				string name = pScriptClass->GetName();
				archive(name, "class", "!Class");
			}
			{
				string author = pScriptClass->GetAuthor();
				if (!author.empty())
				{
					archive(author, "author", "!Author");
				}
			}
			{
				string description = pScriptClass->GetDescription();
				if (!description.empty())
				{
					archive(description, "description", "!Description");
				}
			}
		}
		archive(Serialization::ActionButton(functor(*this, &CEntityObjectAttribute::ShowInSchematyc)), "showInSchematyc", "^Show In Schematyc");
		archive.closeBlock();
	}
}

void CEntityObjectAttribute::ShowInSchematyc()
{
	CryLinkUtils::ExecuteCommand(CryLinkUtils::ECommand::Show, m_guid);
}

const char* CEntityObjectAttribute::ms_szAttributeName = "SchematycEntityObjectAttribute";
} // Schematyc
