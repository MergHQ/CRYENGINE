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
	}
};

CCloudRegistrator g_cloudRegistrator;

CRYREGISTER_CLASS(CCloudEntity);

CCloudEntity::CCloudEntity()
	: m_cloudSlot(-1)
{
}

void CCloudEntity::Initialize()
{
	CDesignerEntityComponent::Initialize();

	GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_CLIENT_ONLY);
}

void CCloudEntity::OnResetState()
{
	IEntity& entity = *GetEntity();

	if (m_cloudSlot != -1)
	{
		entity.FreeSlot(m_cloudSlot);
		m_cloudSlot = -1;
	}

	if (!m_cloudFile.empty())
	{
		m_cloudSlot = entity.LoadCloud(-1, m_cloudFile);
	}
	if (m_cloudSlot == -1)
	{
		return;
	}

	entity.SetScale(Vec3(m_scale));

	entity.SetCloudMovementProperties(m_cloudSlot, m_properties);
}
