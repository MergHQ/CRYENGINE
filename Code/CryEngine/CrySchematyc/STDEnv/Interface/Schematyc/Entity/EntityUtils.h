// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Action.h>
#include <Schematyc/Component.h>

#include "Schematyc/Entity/IEntityComponent.h"

namespace Schematyc
{
namespace EntityUtils
{
// N.B. The following helper methods may only be used by components and actions belonging to entity objects.

inline IEntityComponentWrapper& GetEntityComponent(const IObject& object)
{
	auto* pEntityObject = static_cast<IEntityComponentWrapper*>(object.GetCustomData());
	SCHEMATYC_ENV_ASSERT(pEntityObject);
	return *pEntityObject;
}

inline IEntity& GetEntity(const IObject& object)
{
	IEntity* pEntity = GetEntityComponent(object).GetEntity();
	SCHEMATYC_ENV_ASSERT(pEntity);
	return *pEntity;
}

inline IEntityComponentWrapper& GetEntityComponent(const CComponent& component)
{
	return GetEntityComponent(component.GetObject());
}

inline IEntity& GetEntity(const CComponent& component)
{
	return GetEntity(component.GetObject());
}

inline IEntityComponentWrapper& GetEntityComponent(const CAction& action)
{
	return GetEntityComponent(action.GetObject());
}

inline IEntity& GetEntity(const CAction& action)
{
	return GetEntity(action.GetObject());
}
} // EntityUtils
} // Schematyc
