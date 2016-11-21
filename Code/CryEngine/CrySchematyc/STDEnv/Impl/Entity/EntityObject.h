// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntity.h>
#include <Schematyc/Entity/IEntityObject.h>

namespace Schematyc
{
// Forward declare classes.
class CEntityObjectAttribute;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CEntityObjectAttribute)

class CEntityObject : public IEntityObject
{
public:

	~CEntityObject();

	bool Init(IEntity* pEntity, bool bIsPreview);
	void PostInit();
	void CreateObject(ESimulationMode simulationMode);
	void DestroyObject();
	void ProcessEvent(const SEntityEvent& event);
	void OnPropertiesChange();

	// IEntityObject
	virtual IEntity&                  GetEntity() override;
	virtual EntityEventSignal::Slots& GetEventSignalSlots() override;
	virtual GeomUpdateSignal&         GetGeomUpdateSignal() override;
	// ~IEntityObject

private:

	CEntityObjectAttributePtr GetEntityObjectAttribute();

private:

	IEntity*          m_pEntity = nullptr;
	bool              m_bIsPreview = false;
	IObject*          m_pObject = nullptr;

	EntityEventSignal m_eventSignal;
	GeomUpdateSignal  m_geomUpdateSignal;
	CConnectionScope  m_connectionScope;
};
} // Schematyc
