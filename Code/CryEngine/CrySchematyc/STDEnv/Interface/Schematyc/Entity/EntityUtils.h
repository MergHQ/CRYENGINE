// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Action.h>
#include <Schematyc/Component.h>

#include "Schematyc/Entity/IEntityObject.h"

struct IEntity;

namespace Schematyc
{
namespace EntityUtils
{
// N.B. The following helper methods may only be used by components and actions belonging to entity objects.

inline IEntityObject& GetEntityObject(const IObject& object)
{
	IEntityObject* pEntityObject = static_cast<IEntityObject*>(object.GetCustomData());
	SCHEMATYC_ENV_ASSERT(pEntityObject);
	return *pEntityObject;
}

inline IEntity& GetEntity(const IObject& object)
{
	return GetEntityObject(object).GetEntity();
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
