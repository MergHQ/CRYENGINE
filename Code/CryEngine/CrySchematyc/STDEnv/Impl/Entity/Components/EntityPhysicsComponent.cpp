// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"

#include "EntityPhysicsComponent.h"
#include "AutoRegister.h"
#include "STDModules.h"

#include <Schematyc/Entity/EntityUtils.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Types/ResourceTypes.h>
#include <CryEntitySystem/IEntityProxy.h>
#include <CryPhysics/physinterface.h>

namespace Schematyc
{
	void CEntityPhysicsComponent::SProperties::Serialize(Serialization::IArchive& archive)
	{
		archive(mass, "mass", "Mass");
		archive(density, "density", "Density");
		archive(bStatic, "static", "Static");
		archive(bEnabled, "enabled", "Enabled");
	}
	
//////////////////////////////////////////////////////////////////////////

	void CEntityPhysicsComponent::Run(ESimulationMode simulationMode)
	{
		if (simulationMode == ESimulationMode::Idle)
		{
			SetPhysicalize(false);
		}
	}

	bool CEntityPhysicsComponent::Init()
	{
		EntityUtils::GetEntityObject(*this).GetGeomUpdateSignal().GetSlots().Connect(Schematyc::Delegate::Make(*this, &CEntityPhysicsComponent::OnGeometryChanged), m_connectionScope);
		return true;
	}

	void CEntityPhysicsComponent::Shutdown()
	{
		EntityUtils::GetEntityObject(*this).GetGeomUpdateSignal().GetSlots().Disconnect(m_connectionScope);
		SetPhysicalize(false);
	}

	void CEntityPhysicsComponent::Register(IEnvRegistrar& registrar)
	{
		CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
		{
			auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityPhysicsComponent, "Physic");
			pComponent->SetAuthor(g_szCrytek);
			pComponent->SetDescription("Entity physics component");
			pComponent->SetIcon("icons:schematyc/entity_physics_component.ico");
			pComponent->SetFlags(EEnvComponentFlags::None);
			pComponent->SetProperties(CEntityPhysicsComponent::SProperties());
			scope.Register(pComponent);

			CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
			// Functions
			{				
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityPhysicsComponent::SetEnabled, "4DC3B15A-D143-4161-8C1F-F6FEE627A85A"_schematyc_guid, "SetPhysicsEnabled");
				pFunction->SetAuthor(g_szCrytek);
				pFunction->SetDescription("Enable/disable the physic (without destroying/recreating it)");
				pFunction->SetFlags(EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'ena', "Enabled");
				componentScope.Register(pFunction);
			}

			//// Signals  #TODO
			//{
			//	auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL_TYPE(SPhysicCollisionSignal, "PhysicCollisionSignal");
			//	pSignal->SetAuthor(g_szCrytek);
			//	pSignal->SetDescription("Sent when two physical objects collided");
			//	componentScope.Register(pSignal);
			//}
		}
	}

	SGUID CEntityPhysicsComponent::ReflectSchematycType(CTypeInfo<CEntityPhysicsComponent>& typeInfo)
	{
		return "178287AF-CFD5-4757-93FA-2CDE64FB633C"_schematyc_guid;
	}
	
	void CEntityPhysicsComponent::SetEnabled(bool bEnable)
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		entity.EnablePhysics(bEnable);
	}

	void CEntityPhysicsComponent::OnGeometryChanged()
	{
		SetPhysicalize(true);
	}
	
	void CEntityPhysicsComponent::SetPhysicalize(bool bActive)
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		IEntityPhysicalProxyPtr	pPhysicProxy = crycomponent_cast<IEntityPhysicalProxyPtr>(entity.CreateProxy(ENTITY_PROXY_PHYSICS));
		
		CRY_ASSERT_MESSAGE(pPhysicProxy, "Could not create physics proxy");

		if (bActive)
		{
			const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
			CRY_ASSERT_MESSAGE(pProperties, "No properties provided to create physic object");

			SEntityPhysicalizeParams params;
			params.type = (pProperties->bStatic) ? PE_STATIC : PE_RIGID;
			params.mass = pProperties->mass;
			params.density = pProperties->density;
			pPhysicProxy->Physicalize(params);

			SetEnabled(pProperties->bEnabled);
		}
		else
		{
			IPhysicalEntity* pPhysicalEntity = entity.GetPhysics();
			if (pPhysicalEntity)
			{
				for (int slot = 0, slotCount = entity.GetSlotCount(); slot < slotCount; ++slot)
				{
					pPhysicalEntity->RemoveGeometry(slot);
				}
			}
		}
	}

} //Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityPhysicsComponent::Register)
