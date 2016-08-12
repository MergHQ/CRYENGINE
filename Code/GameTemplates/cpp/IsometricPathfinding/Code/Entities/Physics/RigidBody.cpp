#include "StdAfx.h"
#include "RigidBody.h"

#include "Game/GameFactory.h"

class CRigidBodyRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		auto properties = new SNativeEntityPropertyInfo[CRigidBody::eNumProperties];
		memset(properties, 0, sizeof(SNativeEntityPropertyInfo) * CRigidBody::eNumProperties);

		RegisterEntityPropertyObject(properties, CRigidBody::eProperty_Model, "Model", "", "Sets the object of the entity");

		RegisterEntityProperty<float>(properties, CRigidBody::eProperty_Mass, "Mass", "", "Sets the object's mass", 0, 10000);

		CGameFactory::RegisterNativeEntity<CRigidBody>("RigidBody", "Physics", "", 0, properties, CRigidBody::eNumProperties);
	}
};

CRigidBodyRegistrator g_rigidBodyRegistrator;

void CRigidBody::ProcessEvent(SEntityEvent &event)
{
	switch(event.event)
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
	const char *modelPath = GetPropertyValue(eProperty_Model);
	if(strlen(modelPath) > 0)
	{
		auto &gameObject = *GetGameObject();

		const int geometrySlot = 0;
		LoadMesh(geometrySlot, modelPath);

		SEntityPhysicalizeParams physicalizationParams;
		physicalizationParams.type = PE_RIGID;
		physicalizationParams.mass = GetPropertyFloat(eProperty_Mass);

		GetEntity()->Physicalize(physicalizationParams);
	}
}