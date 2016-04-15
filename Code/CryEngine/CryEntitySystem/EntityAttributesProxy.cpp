// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include "EntityAttributesProxy.h"

#include <CrySerialization/IArchive.h>
#include <CrySerialization/IArchiveHost.h>

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::ProcessEvent(SEntityEvent& event) {}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::Initialize(SComponentInitializer const& inititializer)
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
EEntityProxy CEntityAttributesProxy::GetType()
{
	return ENTITY_PROXY_ATTRIBUTES;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::Release()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::Done() {}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::Update(SEntityUpdateContext& context) {}

//////////////////////////////////////////////////////////////////////////
bool CEntityAttributesProxy::Init(IEntity* pEntity, SEntitySpawnParams& params)
{
	if (params.pArchetype)
	{
		EntityAttributeUtils::CloneAttributes(params.pArchetype->GetAttributes(), m_attributes);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
	if (params.pArchetype)
	{
		EntityAttributeUtils::CloneAttributes(params.pArchetype->GetAttributes(), m_attributes);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::SerializeXML(XmlNodeRef& entityNodeXML, bool loading)
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
void CEntityAttributesProxy::Serialize(TSerialize serialize) {}

//////////////////////////////////////////////////////////////////////////
bool CEntityAttributesProxy::NeedSerialize()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityAttributesProxy::GetSignature(TSerialize signature)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
}

//////////////////////////////////////////////////////////////////////////
void CEntityAttributesProxy::SetAttributes(const TEntityAttributeArray& attributes)
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
TEntityAttributeArray& CEntityAttributesProxy::GetAttributes()
{
	return m_attributes;
}

//////////////////////////////////////////////////////////////////////////
const TEntityAttributeArray& CEntityAttributesProxy::GetAttributes() const
{
	return m_attributes;
}
