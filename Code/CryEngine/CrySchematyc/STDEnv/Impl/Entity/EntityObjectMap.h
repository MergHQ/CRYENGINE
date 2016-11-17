// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Types/BasicTypes.h>

namespace Schematyc
{
typedef CDelegate<EVisitStatus (EntityId, ObjectId)> EntityObjectVisitor;

class CEntityObjectMap
{
private:

	typedef std::unordered_map<EntityId, ObjectId> EntityObjects;

public:

	void         AddEntity(EntityId entityId, ObjectId objectId);
	void         RemoveEntity(EntityId entityId);
	ObjectId     GetEntityObjectId(EntityId entityId) const;
	IObject*     GetEntityObject(EntityId entityId) const;
	EVisitResult VisitEntityObjects(const EntityObjectVisitor& visitor) const;

private:

	EntityObjects m_entityObjects;
};
} // Schematyc
