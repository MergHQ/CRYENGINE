#include "StdAfx.h"
#include "ParticleEntity.h"

#include <Cry3DEngine/I3DEngine.h>
#include <CryParticleSystem/IParticlesPfx2.h>

class CParticleRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("ParticleEffect") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class ParticleEffect, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CDefaultParticleEntity>("ParticleEffect", "Particles", "Particle.bmp");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("ParticleEffect");
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityProperty<bool>(pPropertyHandler, "Active", "bActive", "1", "");

		RegisterEntityProperty<string>(pPropertyHandler, "ParticleEffect", "", "", "");
	}
};

CParticleRegistrator g_particleRegistrator;

CDefaultParticleEntity::CDefaultParticleEntity()
	: m_particleSlot(-1)
{
}

void CDefaultParticleEntity::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

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

void CDefaultParticleEntity::Reset()
{
	IEntity& entity = *GetEntity();

	// Check if we have to unload first
	if (m_particleSlot != -1)
	{
		entity.FreeSlot(m_particleSlot);
		m_particleSlot = -1;
	}

	// Check if the light is active
	if (!GetPropertyBool(eProperty_Active))
		return;

	if (IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(GetPropertyValue(eProperty_EffectName), "ParticleEntity"))
	{
		m_particleSlot = entity.LoadParticleEmitter(-1, pEffect);
	}
}
