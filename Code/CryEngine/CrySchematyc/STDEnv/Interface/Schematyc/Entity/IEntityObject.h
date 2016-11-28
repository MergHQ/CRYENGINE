// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntity.h>

// Forward declare structures.
struct SEntityEvent;

namespace Schematyc
{
typedef CSignal<void (const SEntityEvent&)> EntityEventSignal;
// EntityEventSignal is used to forward the following entity events from the entity game object extension:
//   ENTITY_EVENT_XFORM
//   ENTITY_EVENT_DONE
//   ENTITY_EVENT_HIDE
//   ENTITY_EVENT_UNHIDE
//   ENTITY_EVENT_SCRIPT_EVENT
//   ENTITY_EVENT_ENTERAREA
//   ENTITY_EVENT_LEAVEAREA

typedef CSignal<void ()> GeomUpdateSignal;

struct IEntityObject
{
	virtual ~IEntityObject() {}

	virtual EntityEventSignal::Slots& GetEventSignalSlots() = 0;
	virtual GeomUpdateSignal&         GetGeomUpdateSignal() = 0;
};

struct IEntityObjectComponent : public IEntityObject, public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityObjectComponent, 0xEF65C1F547754454, 0x8C4BF14686F6512B);

	virtual ~IEntityObjectComponent() {}
};
} // Schematyc
