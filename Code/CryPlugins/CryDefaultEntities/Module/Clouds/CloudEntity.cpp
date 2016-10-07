#include "StdAfx.h"
#include "CloudEntity.h"

#include <Cry3DEngine/I3DEngine.h>

class CCloudRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("Cloud") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class Cloud, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CCloudEntity>("Cloud", "Render", "Clouds.bmp");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Cloud");
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityProperty<string>(pPropertyHandler, "CloudFile", "file_CloudFile", "Libs/Clouds/Default.xml", "");

		RegisterEntityProperty<float>(pPropertyHandler, "Scale", "fScale", "1", "", 0.001f, 10000.f);

		{
			SEntityPropertyGroupHelper group(pPropertyHandler, "Movement");

			RegisterEntityProperty<bool>(pPropertyHandler, "AutoMove", "bAutoMove", "0", "");
			RegisterEntityProperty<Vec3>(pPropertyHandler, "Speed", "vector_Speed", "0,0,0", "");
			RegisterEntityProperty<Vec3>(pPropertyHandler, "SpaceLoopBox", "vector_SpaceLoopBox", "2000,2000,2000", "");
			RegisterEntityProperty<float>(pPropertyHandler, "FadeDistance", "fFadeDistance", "0", "", 0, 100000.f);
		}
	}
};

CCloudRegistrator g_cloudRegistrator;

CCloudEntity::CCloudEntity()
	: m_cloudSlot(-1)
{
}

bool CCloudEntity::Init(IGameObject* pGameObject)
{
	if (!CNativeEntityBase::Init(pGameObject))
		return false;

	GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_CLIENT_ONLY);

	return true;
}

void CCloudEntity::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

	switch (event.event)
	{
	// Physicalize on level start for Launcher
	case ENTITY_EVENT_START_LEVEL:
	// Editor specific, reset on game mode change, property change or transform change
	case ENTITY_EVENT_RESET:
	case ENTITY_EVENT_EDITOR_PROPERTY_CHANGED:
	case ENTITY_EVENT_XFORM_FINISHED_EDITOR:
		Reset();
		break;
	}
}

void CCloudEntity::Reset()
{
	IEntity& entity = *GetEntity();

	if (m_cloudSlot != -1)
	{
		entity.FreeSlot(m_cloudSlot);
		m_cloudSlot = -1;
	}

	m_cloudSlot = entity.LoadCloud(-1, GetPropertyValue(eProperty_CloudFile));
	if (m_cloudSlot == -1)
	{
		return;
	}

	entity.SetScale(Vec3(GetPropertyFloat(eProperty_Scale)));

	SCloudMovementProperties cloudMovementProperties;
	cloudMovementProperties.m_autoMove = GetPropertyBool(eProperty_AutoMove);
	cloudMovementProperties.m_speed = GetPropertyVec3(eProperty_Speed);
	cloudMovementProperties.m_spaceLoopBox = GetPropertyVec3(eProperty_SpaceLoopBox);
	cloudMovementProperties.m_fadeDistance = GetPropertyFloat(eProperty_FadeDistance);
	entity.SetCloudMovementProperties(m_cloudSlot, cloudMovementProperties);
}
