// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityLightComponent.h"

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <CrySerialization/Color.h>
#include <CrySerialization/Decorators/Slider.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"

namespace Schematyc
{

bool CEntityLightComponent::Init()
{
	return true;
}

void CEntityLightComponent::Run(ESimulationMode simulationMode)
{
	if (m_bInitOn)
	{
		SwitchOn();
	}
}

void CEntityLightComponent::Shutdown()
{
	FreeSlot();
}

int CEntityLightComponent::GetSlot() const
{
	return m_slot;
}

void CEntityLightComponent::SetTransform(const CTransform& transform)
{
	CComponent::GetTransform() = transform;

	if (CComponent::GetObject().GetSimulationMode() != ESimulationMode::Idle)
	{
		EntityUtils::GetEntity(*this).SetSlotLocalTM(m_slot, transform.ToMatrix34());
	}
}

void CEntityLightComponent::Switch(bool bOn)
{
	const bool bPrevOn = m_bOn;
	if (bOn != bPrevOn)
	{
		if (bOn)
		{
			SwitchOn();
		}
		else
		{
			SwitchOff();
		}
	}
}

void CEntityLightComponent::ReflectType(CTypeDesc<CEntityLightComponent>& desc)
{
	desc.SetGUID("ed123a98-462f-49a0-8d1b-362d6449d81a"_schematyc_guid);
	desc.SetLabel("Light");
	desc.SetDescription("Entity light component");
	desc.SetIcon("icons:schematyc/entity_light_component.png");
	desc.SetComponentFlags({ EComponentFlags::Transform, EComponentFlags::Socket, EComponentFlags::Attach });
	desc.AddMember(&CEntityLightComponent::m_bInitOn, 'on', "on", "On", "Initial on/off state", true);
	desc.AddMember(&CEntityLightComponent::m_color, 'col', "color", "Color", "Color", Col_White);
	desc.AddMember(&CEntityLightComponent::m_diffuseMultiplier, 'diff', "diffuseMultiplier", "Diffuse Multiplier", "Diffuse Multiplier", 1.0f);
	desc.AddMember(&CEntityLightComponent::m_specularMultiplier, 'spec', "specularMultiplier", "Specular Multiplier", "Specular Multiplier", 1.0f);
	desc.AddMember(&CEntityLightComponent::m_hdrDynamicMultiplier, 'hdr', "hdrDynamicMultiplier", "HDR Dynamic Multiplier", "HDR Dynamic Multiplier", 1.0f);
	desc.AddMember(&CEntityLightComponent::m_radius, 'rad', "radius", "Radius", "Radius", 10.0f);
	desc.AddMember(&CEntityLightComponent::m_frustumAngle, 'ang', "frustumAngle", "Frustum Angle", "Frustum Angle", 45.0f);
	desc.AddMember(&CEntityLightComponent::m_attenuationBulbSize, 'bulb', "attenuationBulbSize", "Attenuation Bulb Size", "Attenuation Bulb Size", 0.05f);
}

void CEntityLightComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntityLightComponent));
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityLightComponent::SetTransform, "729721c4-a09e-4903-a8a6-fa69388acfc6"_schematyc_guid, "SetTransform");
			pFunction->SetDescription("Set transform");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'trn', "Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityLightComponent::Switch, "f3d21e93-054d-4df6-ba31-2b44f0d2e1eb"_schematyc_guid, "Switch");
			pFunction->SetDescription("Switch light on/off");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'on', "On");
			componentScope.Register(pFunction);
		}
	}
}

void CEntityLightComponent::SwitchOn()
{
	CComponent* pParent = CComponent::GetParent();

	ColorF color = m_color;
	color.r *= m_diffuseMultiplier;
	color.g *= m_diffuseMultiplier;
	color.b *= m_diffuseMultiplier;

	CDLight light;
	light.SetLightColor(color);
	light.SetSpecularMult(m_specularMultiplier);
	light.m_fHDRDynamic = m_hdrDynamicMultiplier;
	light.m_fRadius = m_radius;
	light.m_fLightFrustumAngle = m_frustumAngle;
	light.m_fAttenuationBulbSize = m_attenuationBulbSize;

	IEntity& entity = EntityUtils::GetEntity(*this);

	m_slot = entity.LoadLight(EmptySlot, &light);
	if (pParent)
	{
		const int parentSlot = pParent->GetSlot();
		if (parentSlot != EmptySlot)
		{
			entity.SetParentSlot(parentSlot, m_slot);
		}
	}
	entity.SetSlotLocalTM(m_slot, CComponent::GetTransform().ToMatrix34());

	m_bOn = true;
}

void CEntityLightComponent::SwitchOff()
{
	FreeSlot();

	m_bOn = false;
}

void CEntityLightComponent::FreeSlot()
{
	if (m_slot != EmptySlot)
	{
		EntityUtils::GetEntity(*this).FreeSlot(m_slot);
		m_slot = EmptySlot;
	}
}

} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityLightComponent::Register)
