// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityParticleEmitterComponent.h"

// TODO: Fix dependencies from IParticles.h - remove following includes block!
#include <CryRenderer/IRenderAuxGeom.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"

namespace Schematyc
{

void CEntityParticleEmitterComponent::SAdvancedProperties::ReflectType(Schematyc::CTypeDesc<CEntityParticleEmitterComponent::SAdvancedProperties>& desc)
{
	desc.SetGUID("da718fa6-6484-4b52-8069-2af728e70b79"_schematyc_guid);
}

// N.B. Non-intrusive serialization is used here only to ensure backward compatibility.
//      If files were patched we could instead reflect members and let the system serialize them automatically.
inline bool Serialize(Serialization::IArchive& archive, CEntityParticleEmitterComponent::SAdvancedProperties& value, const char* szName, const char* szLabel)
{
	if (archive.openBlock("advanced", "Advanced"))
	{
		SpawnParams& params = value.spawnParams;
		archive(params.fSizeScale, "scale", "Uniform Scale");
		archive.doc("Emitter uniform scale");
		archive(params.fCountScale, "countScale", "Count Scale");
		archive.doc("Particle count multiplier");
		archive(params.fSpeedScale, "speedScale", "Speed Scale");
		archive.doc("Particle emission speed multiplier");
		archive(params.fTimeScale, "timeScale", "Time Scale");
		archive.doc("Emitter time multiplier");
		archive(params.fPulsePeriod, "pulsePeriod", "Pulse Period");
		archive.doc("How often to restart emitter");
		archive(params.fStrength, "strength", "Strength");
		archive.doc("Strength curve parameter");
		archive.closeBlock();
	}
	return true;
}

bool CEntityParticleEmitterComponent::Init()
{
	return true;
}

void CEntityParticleEmitterComponent::Run(ESimulationMode simulationMode)
{
	if (!m_effectName.value.empty())
	{
		if (m_slot == EmptySlot)
		{
			LoadParticleEmitter();
		}
		SetVisible(m_bInitVisible);
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

void CEntityParticleEmitterComponent::SetParameters(float uniformScale, float countScale, float speedScale, float timeScale)
{
	IEntity& entity = EntityUtils::GetEntity(*this);
	IParticleEmitter* pEmitter = entity.GetParticleEmitter(m_slot);
	if (pEmitter)
	{
		SpawnParams newSpawnParams = m_advancedProperties.spawnParams;
		newSpawnParams.fSizeScale = uniformScale;
		newSpawnParams.fCountScale = countScale;
		newSpawnParams.fSpeedScale = speedScale;
		newSpawnParams.fTimeScale = timeScale;
		pEmitter->SetSpawnParams(newSpawnParams);
	}
}

void CEntityParticleEmitterComponent::SetAttributeAsFloat(CSharedString attributeName, float value)
{
	IEntity& entity = EntityUtils::GetEntity(*this);
	IParticleEmitter* pEmitter = entity.GetParticleEmitter(m_slot);
	if (pEmitter)
	{
		auto attributeId = pEmitter->GetAttributes().FindAttributeIdByName(attributeName.c_str());
		pEmitter->GetAttributes().SetAsFloat(attributeId, value);
	}
}

void CEntityParticleEmitterComponent::SetAttributeAsInteger(CSharedString attributeName, int value)
{
	IEntity& entity = EntityUtils::GetEntity(*this);
	IParticleEmitter* pEmitter = entity.GetParticleEmitter(m_slot);
	if (pEmitter)
	{
		auto attributeId = pEmitter->GetAttributes().FindAttributeIdByName(attributeName.c_str());
		pEmitter->GetAttributes().SetAsInteger(attributeId, value);
	}
}

void CEntityParticleEmitterComponent::SetAttributeAsBoolean(CSharedString attributeName, bool value)
{
	IEntity& entity = EntityUtils::GetEntity(*this);
	IParticleEmitter* pEmitter = entity.GetParticleEmitter(m_slot);
	if (pEmitter)
	{
		auto attributeId = pEmitter->GetAttributes().FindAttributeIdByName(attributeName.c_str());
		pEmitter->GetAttributes().SetAsBoolean(attributeId, value);
	}
}

void CEntityParticleEmitterComponent::SetAttributeAsColor(CSharedString attributeName, ColorF value)
{
	IEntity& entity = EntityUtils::GetEntity(*this);
	IParticleEmitter* pEmitter = entity.GetParticleEmitter(m_slot);
	if (pEmitter)
	{
		auto attributeId = pEmitter->GetAttributes().FindAttributeIdByName(attributeName.c_str());
		pEmitter->GetAttributes().SetAsColor(attributeId, value);
	}
}

void CEntityParticleEmitterComponent::SetSpawnGeom(const GeomFileName& geomName)
{
	IStatObj* pObject = gEnv->p3DEngine->LoadStatObj(geomName.value.c_str());
	m_advancedProperties.geomRef.Set(pObject);

	IEntity& entity = EntityUtils::GetEntity(*this);
	IParticleEmitter* pEmitter = entity.GetParticleEmitter(m_slot);
	if (pEmitter)
		pEmitter->SetEmitGeom(m_advancedProperties.geomRef);
}

void CEntityParticleEmitterComponent::ReflectType(CTypeDesc<CEntityParticleEmitterComponent>& desc)
{
	desc.SetGUID("bf9503cf-d25c-4923-a1cb-8658847aa9a6"_schematyc_guid);
	desc.SetLabel("ParticleEmitter");
	desc.SetDescription("Particle emitter component");
	desc.SetIcon("icons:schematyc/entity_particle_emitter_component.png");
	desc.SetComponentFlags({ EComponentFlags::Transform, EComponentFlags::Socket, EComponentFlags::Attach });
	desc.AddMember(&CEntityParticleEmitterComponent::m_effectName, 'name', "effectName", "Effect", "Effect");
	desc.AddMember(&CEntityParticleEmitterComponent::m_bInitVisible, 'vis', "visible", "Visible", "Effect is initially visible", true);
	// desc.AddMember(&CEntityParticleEmitterComponent::m_bPrime, 'prim', "prime", "Prime", "Advance emitter age to its equilibrium state", false);
	desc.AddMember(&CEntityParticleEmitterComponent::m_advancedProperties, 'adv', "advanced", "Advanced", nullptr);
}

void CEntityParticleEmitterComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntityParticleEmitterComponent));
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetTransform, "97af940e-6a2c-4374-a43d-74d90ec385e2"_schematyc_guid, "SetTransform");
			pFunction->SetDescription("Set particle emitter transform");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'trn', "Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetVisible, "c9ac7f56-e6d2-4461-8871-54fb58d30e62"_schematyc_guid, "SetVisible");
			pFunction->SetDescription("Show/hide particle emitter");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'vis', "Visible");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::IsVisible, "ba91ef70-02fc-4171-b8a0-637f16e3321d"_schematyc_guid, "IsVisible");
			pFunction->SetDescription("Is particle emitter visible?");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'vis', "Visible");
			componentScope.Register(pFunction);
		}
		{
			const float one = 1.0f;
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetParameters, "AC32295E-021A-409F-8B4B-FB5ACF0398BE"_schematyc_guid, "SetParameters");
			pFunction->SetDescription("Changes particle emitter parameters during runtime");			
			pFunction->BindInput(1, 'usc', "Uniform Scale", "Uniform Scale", one);
			pFunction->BindInput(2, 'csc', "Count Scale", "Count Scale", one);
			pFunction->BindInput(3, 'ssc', "Speed Scale", "Speed Scale", one);
			pFunction->BindInput(4, 'tsc', "Time Scale", "Time Scale", one);
			componentScope.Register(pFunction);
		}
		{
			const float zero = 0.0f;
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetAttributeAsFloat, "9348B0D3-1D88-4763-ABA0-D73A1673AA07"_schematyc_guid, "SetAttributeAsFloat");
			pFunction->SetDescription("Changes particle emitter attribute during runtime");
			pFunction->BindInput(1, 'attr', "Attribute", "Attribute");
			pFunction->BindInput(2, 'val', "Value", "Value", zero);
			componentScope.Register(pFunction);
		}
		{
			const int zero = 0;
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetAttributeAsInteger, "4AD980DC-6362-4AF2-A5DC-C94C18D2C025"_schematyc_guid, "SetAttributeAsInteger");
			pFunction->SetDescription("Changes particle emitter attribute during runtime");
			pFunction->BindInput(1, 'attr', "Attribute", "Attribute");
			pFunction->BindInput(2, 'val', "Value", "Value", zero);
			componentScope.Register(pFunction);
		}
		{
			const bool defaultVal = true;
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetAttributeAsBoolean, "6CD2B233-CB17-4FD7-BAC4-08986D2A14E1"_schematyc_guid, "SetAttributeAsBoolean");
			pFunction->SetDescription("Changes particle emitter attribute during runtime");
			pFunction->BindInput(1, 'attr', "Attribute", "Attribute");
			pFunction->BindInput(2, 'val', "Value", "Value", defaultVal);
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetAttributeAsColor, "BA761369-37B2-4B13-9531-027644B39D7D"_schematyc_guid, "SetAttributeAsColor");
			pFunction->SetDescription("Changes particle emitter attribute during runtime");
			pFunction->BindInput(1, 'attr', "Attribute", "Attribute");
			pFunction->BindInput(2, 'val', "Value", "Value", Col_White);
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityParticleEmitterComponent::SetSpawnGeom, "CD20B7A2-9200-4026-B221-BA53E02F20E2"_schematyc_guid, "SetSpawnGeometry");
			pFunction->SetDescription("Sets the geometry to be used for emitters that spawn from geometry");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'geom', "Geometry", "Geometry");
			componentScope.Register(pFunction);
		}
	}
}

void CEntityParticleEmitterComponent::LoadParticleEmitter()
{
	IParticleEffect* pParticleEffect = gEnv->pParticleManager->FindEffect(m_effectName.value.c_str(), "Schematyc::GameEntity::CEntityParticleEmitterComponent::LoadParticleEmitter");
	if (pParticleEffect)
	{
		SpawnParams spawnParams = m_advancedProperties.spawnParams;
		if (!m_bInitVisible)
			spawnParams.fCountScale = 0.0f;

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
