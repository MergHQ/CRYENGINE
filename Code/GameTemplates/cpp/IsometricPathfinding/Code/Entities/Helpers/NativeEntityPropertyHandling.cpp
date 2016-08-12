#include "StdAfx.h"
#include "NativeEntityPropertyHandling.h"

#include <CryEntitySystem/IEntitySystem.h>

CNativeEntityPropertyHandler::CNativeEntityPropertyHandler(SNativeEntityPropertyInfo *pProperties, int numProperties, uint32 scriptFlags)
	: m_pProperties(pProperties)
	, m_numProperties(numProperties)
	, m_scriptFlags(scriptFlags) 
{
	CRY_ASSERT(m_pProperties != nullptr);

	// Make sure that our OnRemove function is called so that we can erase from the property map
	gEnv->pEntitySystem->AddSink(this, IEntitySystem::OnRemove, 0);
}

CNativeEntityPropertyHandler::~CNativeEntityPropertyHandler()
{
	delete[] m_pProperties;
}

bool CNativeEntityPropertyHandler::OnRemove(IEntity *pEntity)
{
	m_entityPropertyMap.erase(pEntity);

	return true;
}

void CNativeEntityPropertyHandler::LoadEntityXMLProperties(IEntity *pEntity, const XmlNodeRef& xml)
{
	if(auto properties = xml->findChild("Properties"))
	{
		LOADING_TIME_PROFILE_SECTION;

		// Find (or insert) property storage entry for this entity
		auto entityPropertyStorageIt = m_entityPropertyMap.find(pEntity);
		if (entityPropertyStorageIt == m_entityPropertyMap.end())
		{
			// Create the entry and reserve bucket count
			entityPropertyStorageIt = m_entityPropertyMap.insert(TEntityPropertyMap::value_type(pEntity, CNativeEntityPropertyHandler::TPropertyStorage(m_numProperties))).first;
		}

		// Load default group of properties
		LoadEntityXMLGroupProperties(entityPropertyStorageIt->second, properties, true);

		// Load folders
		for(auto i = 0; i < properties->getChildCount(); i++)
		{
			LoadEntityXMLGroupProperties(entityPropertyStorageIt->second, properties->getChild(i), false);
		}
	}

	PropertiesChanged(pEntity);
}

void CNativeEntityPropertyHandler::LoadEntityXMLGroupProperties(TPropertyStorage &entityProperties, const XmlNodeRef &groupNode, bool bRootNode)
{
	bool bFoundGroup = bRootNode;

	for(auto i = 0; i < m_numProperties; i++)
	{
		auto &info = m_pProperties[i].info;

		if(!bFoundGroup)
		{
			if(!strcmp(info.name, groupNode->getTag()) && info.type == IEntityPropertyHandler::FolderBegin)
			{
				bFoundGroup = true;
				continue;
			}
		}
		else
		{
			if(info.type == IEntityPropertyHandler::FolderEnd || info.type == IEntityPropertyHandler::FolderBegin)
				break;

			XmlString value;
			if (groupNode->getAttr(info.name, value))
			{
				entityProperties[i] = value;
			}
		}
	}
}

void CNativeEntityPropertyHandler::SetProperty(IEntity *pEntity, int index, const char *value)
{
	CRY_ASSERT(index < m_numProperties);

	auto entityPropertyStorageIt = m_entityPropertyMap.find(pEntity);
	if (entityPropertyStorageIt == m_entityPropertyMap.end())
	{
		entityPropertyStorageIt = m_entityPropertyMap.insert(TEntityPropertyMap::value_type(pEntity, CNativeEntityPropertyHandler::TPropertyStorage(index + 1))).first;
	}

	// Update this entity's property at index
	entityPropertyStorageIt->second[index] = value;
}

const char *CNativeEntityPropertyHandler::GetProperty(IEntity *pEntity, int index) const
{
	CRY_ASSERT(index < m_numProperties);

	auto entityPropertyStorageIt = m_entityPropertyMap.find(pEntity);
	if (entityPropertyStorageIt != m_entityPropertyMap.end())
	{
		auto entityPropertyMap = entityPropertyStorageIt->second.find(index);
		if(entityPropertyMap != entityPropertyStorageIt->second.end())
			return entityPropertyMap->second;
	}

	return m_pProperties[index].defaultValue;
}

void CNativeEntityPropertyHandler::PropertiesChanged(IEntity *pEntity)
{
	auto propertiesChangedEvent = SEntityEvent(ENTITY_EVENT_EDITOR_PROPERTY_CHANGED);
	pEntity->SendEvent(propertiesChangedEvent);
}