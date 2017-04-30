// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DetailWidget.h"

#include <CrySerialization/IArchiveHost.h>
#include <Schematyc/SerializationUtils/ISerializationContext.h>

namespace CrySchematycEditor {

class CEmptyDetailItem : public IDetailItem
{
public:
	// IDetailItem
	virtual EDetailItemType              GetType() const override                             { return EDetailItemType::Empty; }
	virtual Schematyc::SGUID             GetGUID() const override                             { return Schematyc::SGUID(); }
	virtual Serialization::CContextList* GetContextList() override                            { return nullptr; }
	virtual void                         Serialize(Serialization::IArchive& archive) override {}
	// ~IDetailItem
};

CSettingsDetailItem::CSettingsDetailItem(const Schematyc::ISettingsPtr& pSettings)
	: m_pSettings(pSettings)
{}

EDetailItemType CSettingsDetailItem::GetType() const
{
	return EDetailItemType::Settings;
}

Schematyc::SGUID CSettingsDetailItem::GetGUID() const
{
	return Schematyc::SGUID();
}

Serialization::CContextList* CSettingsDetailItem::GetContextList()
{
	return nullptr;
}

void CSettingsDetailItem::Serialize(Serialization::IArchive& archive)
{
	if (m_pSettings)
	{
		m_pSettings->Serialize(archive);
	}
}

CScriptElementDetailItem::CScriptElementDetailItem(Schematyc::IScriptElement* pScriptElement)
	: m_pScriptElement(pScriptElement)
	, m_pContextList(new Serialization::CContextList())
{}

EDetailItemType CScriptElementDetailItem::GetType() const
{
	return EDetailItemType::ScriptElement;
}

Schematyc::SGUID CScriptElementDetailItem::GetGUID() const
{
	return m_pScriptElement ? m_pScriptElement->GetGUID() : Schematyc::SGUID();
}

Serialization::CContextList* CScriptElementDetailItem::GetContextList()
{
	return m_pContextList.get();
}

void CScriptElementDetailItem::Serialize(Serialization::IArchive& archive)
{
	if (m_pScriptElement)
	{
		Schematyc::SSerializationContextParams serParams(archive, Schematyc::ESerializationPass::Edit);
		Schematyc::ISerializationContextPtr pSerializationContext = gEnv->pSchematyc->CreateSerializationContext(serParams);

		m_pScriptElement->Serialize(archive);
		if (archive.isInput())
		{
			gEnv->pSchematyc->GetScriptRegistry().ElementModified(*m_pScriptElement); // #SchematycTODO : Call from script element? YES!
		}
	}
}

}
