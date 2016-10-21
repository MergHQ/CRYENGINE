// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Schematyc_DetailWidget.h"

#include <CrySerialization/IArchiveHost.h>
#include <Schematyc/SerializationUtils/Schematyc_ISerializationContext.h>

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
		Schematyc::ISerializationContextPtr pSerializationContext = GetSchematycFramework().CreateSerializationContext(serParams);

		m_pScriptElement->Serialize(archive);
		if (archive.isInput())
		{
			GetSchematycFramework().GetScriptRegistry().ElementModified(*m_pScriptElement); // #SchematycTODO : Call from script element? YES!
		}
	}
}

CScriptGraphViewNodeDetailItem::CScriptGraphViewNodeDetailItem(const Schematyc::CScriptGraphViewNodePtr& pScriptGraphViewNode)
	: m_pScriptGraphViewNode(pScriptGraphViewNode)
	, m_pContextList(new Serialization::CContextList())
{}

EDetailItemType CScriptGraphViewNodeDetailItem::GetType() const
{
	return EDetailItemType::GraphViewNode;
}

Schematyc::SGUID CScriptGraphViewNodeDetailItem::GetGUID() const
{
	return m_pScriptGraphViewNode ? m_pScriptGraphViewNode->GetGUID() : Schematyc::SGUID();
}

Serialization::CContextList* CScriptGraphViewNodeDetailItem::GetContextList()
{
	return m_pContextList.get();
}

void CScriptGraphViewNodeDetailItem::Serialize(Serialization::IArchive& archive)
{
	if (m_pScriptGraphViewNode)
	{
		Schematyc::SSerializationContextParams serParams(archive, Schematyc::ESerializationPass::Edit);
		Schematyc::ISerializationContextPtr pSerializationContext = GetSchematycFramework().CreateSerializationContext(serParams);

		m_pScriptGraphViewNode->Serialize(archive);
		if (archive.isInput())
		{
			GetSchematycFramework().GetScriptRegistry().ElementModified(m_pScriptGraphViewNode->GetScriptGraphNode().GetGraph().GetElement());   // #SchematycTODO : Call from script element? YES!
		}
	}
}

}
