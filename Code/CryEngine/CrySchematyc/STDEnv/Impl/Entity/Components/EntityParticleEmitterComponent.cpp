// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityParticleEmitterComponent.h"

// TODO: Fix dependencies from IParticles.h - remove following includes block!
#include <Cry3DEngine/I3DEngine.h>
#include <CryParticleSystem/IParticles.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"

namespace Schematyc
{
CEntityParticleEmitterComponent::SProperties::SProperties()
	: scale(1.0f)
	, bVisible(true)
	, bPrime(false)
	, countScale(1.0f)
	, speedScale(1.0f)
	, timeScale(1.0f)
	, pulsePeriod(0.0f)
	, strength(-1.0f)
{}

void CEntityParticleEmitterComponent::SProperties::Serialize(Serialization::IArchive& archive)
{
	archive(effectName, "effectName", "Effect");
	archive.doc("Effect");
	archive(bVisible, "visible", "Visible");
	archive.doc("Effect is initially visible");
	archive(bPrime, "prime", "Prime");
	archive.doc("Advance emitter age to its equilibrium state");

	if (archive.openBlock("advanced", "Advanced"))
	{
		archive(scale, "scale", "Uniform Scale");
		archive.doc("Emitter uniform scale");
		archive(countScale, "countScale", "Count Scale");
		archive.doc("Particle count multiplier");
		archive(speedScale, "speedScale", "Speed Scale");
		archive.doc("Particle emission speed multiplier");
		archive(timeScale, "timeScale", "Time Scale");
		archive.doc("Emitter time multiplier");
		archive(pulsePeriod, "pulsePeriod", "Pulse Period");
		archive.doc("How often to restart emitter");
		archive(strength, "strength", "Strength");
		archive.doc("Strength curve parameter");
		archive.closeBlock();
	}
}

bool CEntityParticleEmitterComponent::Init()
{
	return true;
}

void CEntityParticleEmitterComponent::Run(ESimulationMode simulationMode)
{
	const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
	if (!pProperties->effectName.value.empty())
	{
		if (m_slot == EmptySlot)
		{
			LoadParticleEmitter();
		}
		SetVisible(pProperties->bVisible);
	}
}

void CEntityParticleEmitterComponent::Shutdown()
{
	if (m_slot != EmptySlot)
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		IParticleEmitter* const pParticleEmitter = entity.GetParticleEmitter(m_slot);
		if (pParticleEmitter)
		{
			pParticleEmitter->Kill();
		}
		entity.FreeSlot(m_slot);
		m_slot = EmptySlot;
	}
}

int CEntityParticleEmitterComponent::GetSlot() const
{
	return m_slot;
}

void CEntityParticleEmitterComponent::SetTransform(const CTransform& transform)
{
	CComponent::GetTransform() = transform;

	if (CComponent::GetObject().GetSimulationMode() != ESimulationMode::Idle)
	{
		EntityUtils::GetEntity(*this).SetSlotLocalTM(m_slot, transform.ToMatrix34());
	}
}

void CEntityParticleEmitterComponent::SetVisible(bool bVisible)
{
	if (bVisible != m_bVisible)
	{
		IParticleEmitter* pParticleEmitter = EntityUtils::GetEntity(*this).GetParticleEmitter(m_slot);
		if (pParticleEmitter)
		{
			pParticleEmitter->Activate(bVisible);
			if (bVisible)
			{
				pParticleEmitter->Restart();
			}
			m_bVisible = bVisible;
		}
		else
		{
			m_bVisible = false;
		}
	}
}

bool CEntityParticleEmitterComponent::IsVisible() const
{
	return m_bVisible;
}

SGUID CEntityParticleEmitterComponent::ReflectSchematycType(CTypeInfo<CEntityParticleEmitterComponent>& typeInfo)
{
	return "bf9503cf-d25c-4923-a1cb-8658847aa9a6"_schematyc_guid;
}

void CEntityParticleEmitterComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityParticleEmitterComponent, "ParticleEmitter");
		pComponent->SetAuthor(g_szCrytek);
		pComponent->SetDescription("Particle emitter component");
		pComponent->SetIcon("icons:schematyc/entity_particle_emitter_component.png");
		pComponent->SetFlags({ EEnvComponentFlags::Transform, EEnvComponentFlags::Socket, EEnvComponentFlags::Attach });
		pComponent->SetProperties(SProperties());
		scope.Register(pComponent);

		CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetTransform, "97af940e-6a2c-4374-a43d-74d90ec385e2"_schematyc_guid, "SetTransform");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Set particle emitter transform");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'trn', "Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetVisible, "c9ac7f56-e6d2-4461-8871-54fb58d30e62"_schematyc_guid, "SetVisible");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Show/hide particle emitter");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'vis', "Visible");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::IsVisible, "ba91ef70-02fc-4171-b8a0-637f16e3321d"_schematyc_guid, "IsVisible");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Is particle emitter visible?");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'vis', "Visible");
			componentScope.Register(pFunction);
		}
	}
}

void CEntityParticleEmitterComponent::LoadParticleEmitter()
{
	const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
	IParticleEffect* pParticleEffect = gEnv->pParticleManager->FindEffect(pProperties->effectName.value.c_str(), "Schematyc::GameEntity::CEntityParticleEmitterComponent::LoadParticleEmitter");
	if (pParticleEffect)
	{
		SpawnParams spawnParams;

		spawnParams.fCountScale = pProperties->bVisible ? pProperties->countScale : 0.0f;
		spawnParams.fSpeedScale = pProperties->speedScale;
		spawnParams.fTimeScale = pProperties->timeScale;
		spawnParams.fPulsePeriod = pProperties->pulsePeriod;
		spawnParams.fStrength = pProperties->strength;
		spawnParams.bPrime = pProperties->bPrime;

		IEntity& entity = EntityUtils::GetEntity(*this);
		m_slot = entity.LoadParticleEmitter(m_slot, pParticleEffect, &spawnParams);

		CComponent* pParent = CComponent::GetParent();
		if (pParent)
		{
			entity.SetParentSlot(pParent->GetSlot(), m_slot);
		}
		entity.SetSlotLocalTM(m_slot, CComponent::GetTransform().ToMatrix34());

		m_bVisible = true;
	}
	else
	{
		m_bVisible = false;
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityParticleEmitterComponent::Register)
