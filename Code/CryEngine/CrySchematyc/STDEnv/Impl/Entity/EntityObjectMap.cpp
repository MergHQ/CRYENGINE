// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObjectMap.h"

namespace Schematyc
{
void CEntityObjectMap::AddEntity(EntityId entityId, ObjectId objectId)
{
	m_entityObjects.reserve(200);
	m_entityObjects.insert(EntityObjects::value_type(entityId, objectId));
}
void CEntityObjectMap::RemoveEntity(EntityId entityId)
{
	m_entityObjects.erase(entityId);
}

ObjectId CEntityObjectMap::GetEntityObjectId(EntityId entityId) const
{
	EntityObjects::const_iterator itEntityObject = m_entityObjects.find(entityId);
	return itEntityObject != m_entityObjects.end() ? itEntityObject->second : ObjectId();
}

IObject* CEntityObjectMap::GetEntityObject(EntityId entityId) const
{
	return GetSchematycCore().GetObject(GetEntityObjectId(entityId));
}

EVisitResult CEntityObjectMap::VisitEntityObjects(const EntityObjectVisitor& visitor) const
{
	SCHEMATYC_ENV_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const EntityObjects::value_type& entityObject : m_entityObjects)
		{
			const EVisitStatus status = visitor(entityObject.first, entityObject.second);
			switch (status)
			{
			case EVisitStatus::Stop:
			{
				return EVisitResult::Stopped;
			}
			case EVisitStatus::Error:
			{
				return EVisitResult::Error;
			}
			}
		}
		return EVisitResult::Complete;
	}
	return EVisitResult::Error;
}
} // Schematyc
