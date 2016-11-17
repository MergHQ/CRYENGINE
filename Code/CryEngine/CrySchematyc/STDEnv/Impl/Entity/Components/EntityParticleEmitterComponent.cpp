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
CEntityParticleEmitterComponent::SParticleEmitterProperties::SParticleEmitterProperties()
	: scale(1.0f)
	, bVisible(true)
	, bPrime(false)
	, countScale(1.0f)
	, speedScale(1.0f)
	, timeScale(1.0f)
	, pulsePeriod(0.0f)
	, strength(-1.0f)
{}

void CEntityParticleEmitterComponent::SParticleEmitterProperties::Serialize(Serialization::IArchive& archive)
{
	archive(effectName, "effectName", "Effect");
	archive.doc("Effect");
	archive(bVisible, "visible", "bVisible");
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

void CEntityParticleEmitterComponent::SProperties::Serialize(Serialization::IArchive& archive)
{
	Serialization::SContext propertiesContext(archive, static_cast<const CEntityParticleEmitterComponent::SProperties*>(this));
	archive(particleEmitter, "particleEmitter", "Particle Emitter");
}

CEntityParticleEmitterComponent::SPreviewProperties::SPreviewProperties()
	: bShowGizmos(false)
	, gizmoLength(1.0f)
{}

void CEntityParticleEmitterComponent::SPreviewProperties::Serialize(Serialization::IArchive& archive)
{
	archive(bShowGizmos, "bShowGizmos", "Show Gizmos");
	archive(gizmoLength, "gizmoLength", "Gizmo Length");
}

void CEntityParticleEmitterComponent::CPreviewer::SerializeProperties(Serialization::IArchive& archive)
{
	archive(m_properties, "properties", "Particle Emitter Component");
}

void CEntityParticleEmitterComponent::CPreviewer::Render(const IObject& object, const CComponent& component, const SRendParams& params, const SRenderingPassInfo& passInfo) const
{
	if (m_properties.bShowGizmos)
	{
		static_cast<const CEntityParticleEmitterComponent&>(component).RenderGizmo(m_properties.gizmoLength);
	}
}

CEntityParticleEmitterComponent::CEntityParticleEmitterComponent()
	: m_slot(EmptySlot)
{}

bool CEntityParticleEmitterComponent::Init()
{
	return true;
}

void CEntityParticleEmitterComponent::Run(ESimulationMode simulationMode)
{
	SProperties* pProperties = static_cast<SProperties*>(CComponent::GetProperties());
	if (!pProperties->particleEmitter.effectName.value.empty())
	{
		if (m_slot == EmptySlot)
		{
			LoadParticleEmitter();
		}
		SetVisible(pProperties->particleEmitter.bVisible);
	}
}

void CEntityParticleEmitterComponent::Shutdown()
{
	if (m_slot != EmptySlot)
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		entity.GetParticleEmitter(m_slot)->Kill();
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
	SProperties* pProperties = static_cast<SProperties*>(CComponent::GetProperties());
	if (bVisible != pProperties->particleEmitter.bVisible)
	{
		if (CComponent::GetObject().GetSimulationMode() != ESimulationMode::Idle)
		{
			IParticleEmitter* pParticleEmitter = EntityUtils::GetEntity(*this).GetParticleEmitter(m_slot);
			SCHEMATYC_ENV_ASSERT(pParticleEmitter);
			if (pParticleEmitter)
			{
				pParticleEmitter->Activate(bVisible);
				if (bVisible)
				{
					pParticleEmitter->Restart();
				}
			}
		}

		pProperties->particleEmitter.bVisible = bVisible;
	}
}

bool CEntityParticleEmitterComponent::IsVisible() const
{
	const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
	return pProperties->particleEmitter.bVisible;
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
		pComponent->SetAuthor("Achim Lang");
		pComponent->SetDescription("Particle emitter component");
		pComponent->SetIcon("icons:schematyc/entity_particle_emitter_component.png");
		pComponent->SetFlags({ EEnvComponentFlags::Transform, EEnvComponentFlags::Socket, EEnvComponentFlags::Attach });
		pComponent->SetProperties(SProperties());
		pComponent->SetPreviewer(CPreviewer());
		scope.Register(pComponent);

		CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetTransform, "97af940e-6a2c-4374-a43d-74d90ec385e2"_schematyc_guid, "SetTransform");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Set particle emitter transform");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'trn', "Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetVisible, "c9ac7f56-e6d2-4461-8871-54fb58d30e62"_schematyc_guid, "SetVisible");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Show/hide particle emitter");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'vis', "Visible");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::IsVisible, "ba91ef70-02fc-4171-b8a0-637f16e3321d"_schematyc_guid, "IsVisible");
			pFunction->SetAuthor("Paul Slinger");
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
	IParticleEffect* pParticleEffect = gEnv->pParticleManager->FindEffect(pProperties->particleEmitter.effectName.value.c_str(), "Schematyc::GameEntity::CEntityParticleEmitterComponent::LoadParticleEmitter");
	if (pParticleEffect)
	{
		SpawnParams spawnParams;

		spawnParams.fCountScale = pProperties->particleEmitter.bVisible ? pProperties->particleEmitter.countScale : 0.0f;
		spawnParams.fSpeedScale = pProperties->particleEmitter.speedScale;
		spawnParams.fTimeScale = pProperties->particleEmitter.timeScale;
		spawnParams.fPulsePeriod = pProperties->particleEmitter.pulsePeriod;
		spawnParams.fStrength = pProperties->particleEmitter.strength;
		spawnParams.bPrime = pProperties->particleEmitter.bPrime;

		IEntity& entity = EntityUtils::GetEntity(*this);
		m_slot = entity.LoadParticleEmitter(m_slot, pParticleEffect, &spawnParams);

		CComponent* pParent = CComponent::GetParent();
		if (pParent)
		{
			entity.SetParentSlot(pParent->GetSlot(), m_slot);
		}
		entity.SetSlotLocalTM(m_slot, CComponent::GetTransform().ToMatrix34());
	}
}

void CEntityParticleEmitterComponent::RenderGizmo(float gizmoLength) const
{
	if (m_slot != EmptySlot)
	{
		IRenderAuxGeom& renderAuxGeom = *gEnv->pRenderer->GetIRenderAuxGeom();
		const Matrix34 worldTM = EntityUtils::GetEntity(*this).GetSlotWorldTM(m_slot);
		const float lineThickness = 4.0f;

		renderAuxGeom.DrawLine(worldTM.GetTranslation(), ColorB(255, 0, 0, 255), worldTM.GetTranslation() + (worldTM.GetColumn0().GetNormalized() * gizmoLength), ColorB(255, 0, 0, 255), lineThickness);
		renderAuxGeom.DrawLine(worldTM.GetTranslation(), ColorB(0, 255, 0, 255), worldTM.GetTranslation() + (worldTM.GetColumn1().GetNormalized() * gizmoLength), ColorB(0, 255, 0, 255), lineThickness);
		renderAuxGeom.DrawLine(worldTM.GetTranslation(), ColorB(0, 0, 255, 255), worldTM.GetTranslation() + (worldTM.GetColumn2().GetNormalized() * gizmoLength), ColorB(0, 0, 255, 255), lineThickness);
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityParticleEmitterComponent::Register)
