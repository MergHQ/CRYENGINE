#include "stdafx.h"
#include "EntityPropertyHandler.h"

#include <CryEntitySystem/IEntitySystem.h>

CEntityPropertyHandler::CEntityPropertyHandler()
{
}

CEntityPropertyHandler::~CEntityPropertyHandler()
{
	if (m_entityPropertyMap.size() > 0)
	{
		gEnv->pEntitySystem->RemoveSink(this);
	}
}

bool CEntityPropertyHandler::OnRemove(IEntity *pEntity)
{
	// Remove the sink listener when there aren't any entities left
	if (m_entityPropertyMap.erase(pEntity) != 0 && m_entityPropertyMap.size() == 0)
	{
		gEnv->pEntitySystem->RemoveSink(this);
	}

	return true;
}

void CEntityPropertyHandler::LoadEntityXMLProperties(IEntity *pEntity, const XmlNodeRef& xml)
{
	LOADING_TIME_PROFILE_SECTION;

	// Find (or insert) property storage entry for this entity
	auto entityPropertyStorageIt = m_entityPropertyMap.find(pEntity);
	if (entityPropertyStorageIt == m_entityPropertyMap.end())
	{
		AddSinkListener();

		// Create the entry and reserve bucket count
		entityPropertyStorageIt = m_entityPropertyMap.insert(TEntityPropertyMap::value_type(pEntity, CEntityPropertyHandler::TPropertyStorage(m_propertyDefinitions.size()))).first;
	}

	if (XmlNodeRef propertiesNode = xml->findChild("Properties"))
	{
		// Load default group of properties
		LoadEntityXMLGroupProperties(entityPropertyStorageIt->second, propertiesNode, true);

		// Load folders
		for (auto i = 0; i < propertiesNode->getChildCount(); i++)
		{
			LoadEntityXMLGroupProperties(entityPropertyStorageIt->second, propertiesNode->getChild(i), false);
		}
	}

	PropertiesChanged(pEntity);
}

void CEntityPropertyHandler::LoadEntityXMLGroupProperties(TPropertyStorage &entityProperties, const XmlNodeRef &groupNode, bool bRootNode)
{
	bool bFoundGroup = bRootNode;

	for(auto it = m_propertyDefinitions.begin(); it != m_propertyDefinitions.end(); ++it)
	{
		if(!bFoundGroup)
		{
			if((!strcmp(it->name, groupNode->getTag()) || !strcmp(it->name, groupNode->getTag())) && it->type == IEntityPropertyHandler::FolderBegin)
			{
				bFoundGroup = true;

				continue;
			}
		}
		else if (it->type == IEntityPropertyHandler::FolderEnd || it->type == IEntityPropertyHandler::FolderBegin)
		{
			break;
		}
		else
		{
			XmlString value;
			if (groupNode->getAttr(it->name, value) || (it->legacyName != nullptr && groupNode->getAttr(it->legacyName, value)))
			{
				// Set the property at this index
				entityProperties[(int)std::distance(m_propertyDefinitions.begin(), it)] = value;
			}
		}
	}
}

int CEntityPropertyHandler::RegisterProperty(const SPropertyInfo& info)
{
	// TODO: Notify Sandbox of this changing?
	m_propertyDefinitions.push_back(info);

	return m_propertyDefinitions.size() - 1;
}

void CEntityPropertyHandler::SetProperty(IEntity *pEntity, int index, const char *value)
{
	CRY_ASSERT(index < (int)m_propertyDefinitions.size());

	auto entityPropertyStorageIt = m_entityPropertyMap.find(pEntity);
	if (entityPropertyStorageIt == m_entityPropertyMap.end())
	{
		AddSinkListener();

		entityPropertyStorageIt = m_entityPropertyMap.insert(TEntityPropertyMap::value_type(pEntity, CEntityPropertyHandler::TPropertyStorage(index + 1))).first;
	}

	// Update this entity's property at index
	entityPropertyStorageIt->second[index] = value;
}

const char *CEntityPropertyHandler::GetProperty(IEntity *pEntity, int index) const
{
	CRY_ASSERT(index < (int)m_propertyDefinitions.size());

	auto entityPropertyStorageIt = m_entityPropertyMap.find(pEntity);
	if (entityPropertyStorageIt != m_entityPropertyMap.end())
	{
		auto entityPropertyMap = entityPropertyStorageIt->second.find(index);
		if(entityPropertyMap != entityPropertyStorageIt->second.end())
			return entityPropertyMap->second;
	}

	// Fall back to the default value
	return m_propertyDefinitions[index].defaultValue;
}

void CEntityPropertyHandler::PropertiesChanged(IEntity *pEntity)
{
	auto propertiesChangedEvent = SEntityEvent(ENTITY_EVENT_EDITOR_PROPERTY_CHANGED);
	pEntity->SendEvent(propertiesChangedEvent);
}

void CEntityPropertyHandler::AddSinkListener()
{
	if (m_entityPropertyMap.size() == 0)
	{
		// Make sure that our OnRemove function is called so that we can erase from the property map
		gEnv->pEntitySystem->AddSink(this, IEntitySystem::OnRemove, 0);
	}
}