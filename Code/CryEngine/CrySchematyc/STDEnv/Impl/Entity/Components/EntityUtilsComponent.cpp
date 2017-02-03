// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityUtilsComponent.h"

#include <CryRenderer/IRenderAuxGeom.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{

bool CEntityUtilsComponent::Init()
{
	return true;
}

void CEntityUtilsComponent::Run(ESimulationMode simulationMode) {}

void  CEntityUtilsComponent::Shutdown() {}

ExplicitEntityId CEntityUtilsComponent::GetEntityId() const
{
	return ExplicitEntityId(EntityUtils::GetEntity(*this).GetId());
}

void CEntityUtilsComponent::ReflectType(CTypeDesc<CEntityUtilsComponent>& desc)
{
	desc.SetGUID("e88093df-904f-4c52-af38-911e26777cdc"_schematyc_guid);
	desc.SetLabel("Utils");
	desc.SetDescription("Entity utilities component");
	desc.SetIcon("icons:schematyc/entity_utils_component.ico");
	desc.SetComponentFlags(EComponentFlags::Singleton);
}

void CEntityUtilsComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntityUtilsComponent));
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityUtilsComponent::GetEntityId, "c01d8df5-058f-406f-8c4c-8426e856f294"_schematyc_guid, "GetEntityId");
			pFunction->SetDescription("Get entity id");
			pFunction->BindOutput(0, 'id', "EntityId");
			componentScope.Register(pFunction);
		}
	}
}

} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityUtilsComponent::Register)
