#include "StdAfx.h"
#include "RigidBody.h"

#include <CryPhysics/physinterface.h>

class CRigidBodyRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("RigidBody") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class RigidBody, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CRigidBody>("RigidBody", "Physics");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("RigidBody");
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityPropertyObject(pPropertyHandler, "Model", "objModel", "", "Sets the object of the entity");

		RegisterEntityProperty<float>(pPropertyHandler, "Mass", "", "", "Sets the object's mass", 0, 10000);
	}
};

CRigidBodyRegistrator g_rigidBodyRegistrator;

bool CRigidBody::Init(IGameObject* pGameObject)
{
	if (!CNativeEntityBase::Init(pGameObject))
	{
		return false;
	}

	// Physicalize during initialization
	Reset();

	return true;
}

void CRigidBody::ProcessEvent(SEntityEvent& event)
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

void CRigidBody::Reset()
{
	const char* modelPath = GetPropertyValue(eProperty_Model);
	if (strlen(modelPath) > 0)
	{
		auto& gameObject = *GetGameObject();

		const int geometrySlot = 0;
		LoadMesh(geometrySlot, modelPath);

		SEntityPhysicalizeParams physicalizationParams;
		physicalizationParams.type = PE_RIGID;
		physicalizationParams.mass = GetPropertyFloat(eProperty_Mass);

		GetEntity()->Physicalize(physicalizationParams);
	}
}
