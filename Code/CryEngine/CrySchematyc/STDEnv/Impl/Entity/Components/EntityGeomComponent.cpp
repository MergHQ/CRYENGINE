// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityGeomComponent.h"

#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
void CEntityGeomComponent::SProperties::Serialize(Serialization::IArchive& archive)
{
	archive(fileName, "fileName", "FileName");
	archive.doc("Name of .cgf file to load");
}

CEntityGeomComponent::CEntityGeomComponent()
	: m_slot(EmptySlot)
{}

bool CEntityGeomComponent::Init()
{
	return true;
}

void CEntityGeomComponent::Run(ESimulationMode simulationMode)
{
	const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
	if (!pProperties->fileName.value.empty())
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		m_slot = entity.LoadGeometry(m_slot, pProperties->fileName.value.c_str());

		CComponent* pParent = CComponent::GetParent();
		if (pParent)
		{
			entity.SetParentSlot(pParent->GetSlot(), m_slot);
		}

		entity.SetSlotLocalTM(m_slot, CComponent::GetTransform().ToMatrix34());
		
		EntityUtils::GetEntityObject(*this).GetGeomUpdateSignal().Send();
	}
}

void CEntityGeomComponent::Shutdown()
{
	if (m_slot != EmptySlot)
	{
		EntityUtils::GetEntity(*this).FreeSlot(m_slot);
		m_slot = EmptySlot;
	}
}

int CEntityGeomComponent::GetSlot() const
{
	return m_slot;
}

void CEntityGeomComponent::Set(const GeomFileName& fileName)
{
	SProperties* pProperties = static_cast<SProperties*>(CComponent::GetProperties());
	pProperties->fileName = fileName;

	if (CComponent::GetObject().GetSimulationMode() != ESimulationMode::Idle)
	{
		if (!fileName.value.empty())
		{
			m_slot = EntityUtils::GetEntity(*this).LoadGeometry(m_slot, fileName.value.c_str());
		}
		else if(m_slot != EmptySlot)
		{
			EntityUtils::GetEntity(*this).FreeSlot(m_slot);
			m_slot = EmptySlot;
		}
		EntityUtils::GetEntityObject(*this).GetGeomUpdateSignal().Send();
	}
}

void CEntityGeomComponent::SetTransform(const CTransform& transform)
{
	CComponent::GetTransform() = transform;

	if (CComponent::GetObject().GetSimulationMode() != ESimulationMode::Idle)
	{
		EntityUtils::GetEntity(*this).SetSlotLocalTM(m_slot, transform.ToMatrix34());
	}
}

void CEntityGeomComponent::SetVisible(bool bVisible) {}

bool CEntityGeomComponent::IsVisible() const
{
	return true;
}

SGUID CEntityGeomComponent::ReflectSchematycType(CTypeInfo<CEntityGeomComponent>& typeInfo)
{
	return "d2474675-c67c-42b2-af33-5c5ace2d1d8c"_schematyc_guid;
}

void CEntityGeomComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityGeomComponent, "Geom");
		pComponent->SetAuthor("Paul Slinger");
		pComponent->SetDescription("Entity geometry component");
		pComponent->SetIcon("icons:schematyc/entity_geom_component.png");
		pComponent->SetFlags({ EEnvComponentFlags::Transform, EEnvComponentFlags::Socket, EEnvComponentFlags::Attach });
		pComponent->SetProperties(SProperties());
		scope.Register(pComponent);

		CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityGeomComponent::Set, "112b2aec-7c52-44b8-a16d-84c98c70d910"_schematyc_guid, "Set");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Set geometry");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'file', "FileName");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityGeomComponent::SetTransform, "2a8c1345-ffb3-4a3e-ac80-b99154f04b69"_schematyc_guid, "SetTransform");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Set transform");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'trn', "Transform");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityGeomComponent::SetVisible, "abc4938d-a631-4a36-9f10-22cf6dc9dabd"_schematyc_guid, "SetVisible");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Show/hide geometry");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindInput(1, 'vis', "Visible");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityGeomComponent::IsVisible, "5aa5e8f0-b4f4-491d-8074-d8b129500d09"_schematyc_guid, "IsVisible");
			pFunction->SetAuthor("Paul Slinger");
			pFunction->SetDescription("Is geometry visible?");
			pFunction->SetFlags(EEnvFunctionFlags::Construction);
			pFunction->BindOutput(0, 'vis', "Visible");
			componentScope.Register(pFunction);
		}
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityGeomComponent::Register)
