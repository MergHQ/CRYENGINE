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
CEntityLightComponent::SProperties::SProperties()
	: bOn(true)
	, color(Col_White)
	, diffuseMultiplier(1.0f)
	, specularMultiplier(1.0f)
	, hdrDynamicMultiplier(1.0f)
	, radius(10.0f)
	, frustumAngle(45.0f)
	, attenuationBulbSize(0.05f)
{}

void CEntityLightComponent::SProperties::Serialize(Serialization::IArchive& archive)
{
	archive(bOn, "on", "On");
	archive.doc("Initial on/off state");

	archive(color, "color", "Color");
	archive.doc("Color");

	archive(Serialization::Slider(diffuseMultiplier, 0.01f, 10.0f), "diffuseMultiplier", "Diffuse Multiplier");
	archive.doc("Diffuse Multiplier");

	archive(Serialization::Slider(specularMultiplier, 0.01f, 10.0f), "specularMultiplier", "Specular Multiplier");
	archive.doc("Specular Multiplier");

	archive(Serialization::Slider(hdrDynamicMultiplier, 0.01f, 10.0f), "hdrDynamicMultiplier", "HDR Dynamic Multiplier");
	archive.doc("HDR Dynamic Multiplier");

	archive(Serialization::Slider(radius, 0.01f, 50.0f), "radius", "Radius");
	archive.doc("Radius");

	archive(Serialization::Slider(frustumAngle, 0.01f, 90.0f), "frustumAngle", "Frustum Angle");
	archive.doc("Frustum Angle");

	archive(Serialization::Slider(attenuationBulbSize, 0.01f, 5.0f), "attenuationBulbSize", "Attenuation Bulb Size");
	archive.doc("Attenuation Bulb Size");
}

CEntityLightComponent::CEntityLightComponent()
	: m_slot(EmptySlot)
	, m_bOn(false)
{}

bool CEntityLightComponent::Init()
{
	return true;
}

void CEntityLightComponent::Run(ESimulationMode simulationMode)
{
	const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
	if (pProperties->bOn)
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

SGUID CEntityLightComponent::ReflectSchematycType(CTypeInfo<CEntityLightComponent>& typeInfo)
{
	return "ed123a98-462f-49a0-8d1b-362d6449d81a"_schematyc_guid;
}

void CEntityLightComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityLightComponent, "Light");
		pComponent->SetAuthor(g_szCrytek);
		pComponent->SetDescription("Entity light component");
		pComponent->SetIcon("icons:schematyc/entity_light_component.png");
		pComponent->SetFlags({ EEnvComponentFlags::Transform, EEnvComponentFlags::Socket, EEnvComponentFlags::Attach });
		pComponent->SetProperties(SProperties());
		scope.Register(pComponent);

		CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityLightComponent::SetTransform, "729721c4-a09e-4903-a8a6-fa69388acfc6"_schematyc_guid, "SetTransform");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Set transform");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'trn', "Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityLightComponent::Switch, "f3d21e93-054d-4df6-ba31-2b44f0d2e1eb"_schematyc_guid, "Switch");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Switch light on/off");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'on', "On");
			componentScope.Register(pFunction);
		}
	}
}

void CEntityLightComponent::SwitchOn()
{
	const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
	CComponent* pParent = CComponent::GetParent();

	ColorF color = pProperties->color;
	color.r *= pProperties->diffuseMultiplier;
	color.g *= pProperties->diffuseMultiplier;
	color.b *= pProperties->diffuseMultiplier;

	CDLight light;
	light.SetLightColor(color);
	light.SetSpecularMult(pProperties->specularMultiplier);
	light.m_fHDRDynamic = pProperties->hdrDynamicMultiplier;
	light.m_fRadius = pProperties->radius;
	light.m_fLightFrustumAngle = pProperties->frustumAngle;
	light.m_fAttenuationBulbSize = pProperties->attenuationBulbSize;

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
