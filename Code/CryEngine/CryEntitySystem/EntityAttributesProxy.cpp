// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include "EntityAttributesProxy.h"

#include <CrySerialization/IArchive.h>
#include <CrySerialization/IArchiveHost.h>

CRYREGISTER_CLASS(CEntityComponentAttributes);

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAttributes::ProcessEvent(SEntityEvent& event) {}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAttributes::Initialize(SComponentInitializer const& inititializer)
{
	if (m_attributes.empty())
	{
		if (IEntityArchetype* pArchetype = inititializer.m_pEntity->GetArchetype())
		{
			EntityAttributeUtils::CloneAttributes(pArchetype->GetAttributes(), m_attributes);
		}
		else
		{
			EntityAttributeUtils::CloneAttributes(inititializer.m_pEntity->GetClass()->GetEntityAttributes(), m_attributes);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
uint64 CEntityComponentAttributes::GetEventMask() const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
EEntityProxy CEntityComponentAttributes::GetProxyType() const
{
	return ENTITY_PROXY_ATTRIBUTES;
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAttributes::SerializeXML(XmlNodeRef& entityNodeXML, bool loading)
{
	if (loading == true)
	{
		if (XmlNodeRef attributesNodeXML = entityNodeXML->findChild("Attributes"))
		{
			SEntityAttributesSerializer serializer(m_attributes);
			Serialization::LoadXmlNode(serializer, attributesNodeXML);
		}
	}
	else
	{
		if (!m_attributes.empty())
		{
			SEntityAttributesSerializer serializer(m_attributes);
			if (XmlNodeRef attributesNodeXML = Serialization::SaveXmlNode(serializer, "Attributes"))
			{
				entityNodeXML->addChild(attributesNodeXML);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAttributes::GameSerialize(TSerialize serialize) {}

//////////////////////////////////////////////////////////////////////////
bool CEntityComponentAttributes::NeedGameSerialize()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAttributes::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAttributes::SetAttributes(const TEntityAttributeArray& attributes)
{
	const size_t attributeCount = attributes.size();
	m_attributes.resize(attributeCount);
	for (size_t iAttribute = 0; iAttribute < attributeCount; ++iAttribute)
	{
		IEntityAttribute* pSrc = attributes[iAttribute].get();
		IEntityAttribute* pDst = m_attributes[iAttribute].get();
		if ((pDst != NULL) && (strcmp(pSrc->GetName(), pDst->GetName()) == 0))
		{
			Serialization::CloneBinary(*pDst, *pSrc);
		}
		else if (pSrc != NULL)
		{
			m_attributes[iAttribute] = pSrc->Clone();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
TEntityAttributeArray& CEntityComponentAttributes::GetAttributes()
{
	return m_attributes;
}

//////////////////////////////////////////////////////////////////////////
const TEntityAttributeArray& CEntityComponentAttributes::GetAttributes() const
{
	return m_attributes;
}
