// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Action.h>
#include <Schematyc/Component.h>

#include "Schematyc/Entity/IEntityObject.h"

namespace Schematyc
{
namespace EntityUtils
{
// N.B. The following helper methods may only be used by components and actions belonging to entity objects.

inline IEntityObject& GetEntityObject(const IObject& object)
{
	IEntityObjectComponent* pEntityObjectComponent = static_cast<IEntityObjectComponent*>(object.GetCustomData());
	SCHEMATYC_ENV_ASSERT(pEntityObjectComponent);
	return *static_cast<IEntityObject*>(pEntityObjectComponent);
}

inline IEntity& GetEntity(const IObject& object)
{
	IEntityObjectComponent* pEntityObjectComponent = static_cast<IEntityObjectComponent*>(object.GetCustomData());
	SCHEMATYC_ENV_ASSERT(pEntityObjectComponent);
	IEntity* pEntity = pEntityObjectComponent->GetEntity();
	SCHEMATYC_ENV_ASSERT(pEntity);
	return *pEntity;
}

inline IEntityObject& GetEntityObject(const CComponent& component)
{
	return GetEntityObject(component.GetObject());
}

inline IEntity& GetEntity(const CComponent& component)
{
	return GetEntity(component.GetObject());
}

inline IEntityObject& GetEntityObject(const CAction& action)
{
	return GetEntityObject(action.GetObject());
}

inline IEntity& GetEntity(const CAction& action)
{
	return GetEntity(action.GetObject());
}
} // EntityUtils
} // Schematyc
