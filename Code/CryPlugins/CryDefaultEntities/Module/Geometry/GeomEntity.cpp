#include "StdAfx.h"
#include "GeomEntity.h"

#include <CryPhysics/physinterface.h>

class CGeomEntityRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("GeomEntity") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class GeomEntity, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CGeomEntity>("GeomEntity", "Geometry");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("GeomEntity");
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityPropertyObject(pPropertyHandler, "Geometry", "object_Model", "", "Sets the object of the entity");
		RegisterEntityProperty<float>(pPropertyHandler, "Mass", "", "-1", "Sets the mass of the entity, a negative value means we won't physicalize", -1, 100000.f);
	}
};

CGeomEntityRegistrator g_geomEntityRegistrator;

void CGeomEntity::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	// Physicalize on level start for Launcher
	case ENTITY_EVENT_START_LEVEL:
	// Editor specific, physicalize on reset, property change or transform change
	case ENTITY_EVENT_RESET:
	case ENTITY_EVENT_EDITOR_PROPERTY_CHANGED:
	case ENTITY_EVENT_XFORM_FINISHED_EDITOR:
		Reset();
		break;
	}
}

void CGeomEntity::Reset()
{
	const char* modelPath = GetPropertyValue(eProperty_Model);
	if (strlen(modelPath) > 0)
	{
		auto& gameObject = *GetGameObject();

		const int geometrySlot = 0;
		LoadMesh(geometrySlot, modelPath);

		float mass = GetPropertyFloat(eProperty_Mass);
		if (mass > 0)
		{
			SEntityPhysicalizeParams physicalizationParams;
			physicalizationParams.type = PE_STATIC;
			physicalizationParams.mass = mass;

			GetEntity()->Physicalize(physicalizationParams);
		}
	}
}
