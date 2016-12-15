// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"

#include "EntityPhysicsComponent.h"
#include "AutoRegister.h"
#include "STDModules.h"

#include <Schematyc/Entity/EntityUtils.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Types/ResourceTypes.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <CryPhysics/physinterface.h>

namespace Schematyc
{
	SERIALIZATION_ENUM_BEGIN_NESTED(CEntityPhysicsComponent, ePhysicType, "DeviceType")
		SERIALIZATION_ENUM(CEntityPhysicsComponent::ePhysicType_Static, "Static", "Static")
		SERIALIZATION_ENUM(CEntityPhysicsComponent::ePhysicType_Rigid, "Rigid", "Rigid")
		SERIALIZATION_ENUM(CEntityPhysicsComponent::ePhysicType_Living, "Living", "Living")
	SERIALIZATION_ENUM_END()

	void CEntityPhysicsComponent::SProperties::Serialize(Serialization::IArchive& archive)
	{
		archive(mass, "mass", "Mass");
		archive(density, "density", "Density");
		archive(bEnabled, "enabled", "Enabled");
		archive(type, "type", "Type");
		if (type == ePhysicType_Living)
		{
			if (archive.openBlock("collider", "Collision Proxy"))
			{
				archive(colliderHeight, "height", "Height");
				archive(colliderRadius, "radius", "Radius");
				archive(colliderVerticalOffset, "verticalOffset", "VerticalOffset");
				archive(Serialization::Range(friction, 0.01f, 1.0f), "friction", "Friction");
				archive.closeBlock();
			}
		}
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
			auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityPhysicsComponent, "Physics");
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
				pFunction->SetDescription("Enable/disable physics (without destroying/recreating)");
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

		if (bActive)
		{
			const SProperties* pProperties = static_cast<const SProperties*>(CComponent::GetProperties());
			CRY_ASSERT_MESSAGE(pProperties, "No properties provided to create physics object");

			SEntityPhysicalizeParams params;
			pe_player_dimensions playerDim;
			pe_player_dynamics playerDyn;

			if (pProperties->type == ePhysicType_Living)
			{
				params.type = PE_LIVING;
				params.mass = pProperties->mass;
				params.density = pProperties->density;

				IPhysicalEntity* pPhysicalEntity = entity.GetPhysics();
				if (pPhysicalEntity)
				{
					pPhysicalEntity->GetParams(&playerDyn);
				}
				playerDyn.gravity = Vec3(0, 0, -9.81f);
				playerDyn.kAirControl = pProperties->friction;
				playerDyn.kAirResistance = 1.0f;
				playerDyn.mass = pProperties->mass;
				playerDyn.minSlideAngle = 45;
				playerDyn.maxClimbAngle = 40;
				playerDyn.minFallAngle = 50;
				playerDyn.maxVelGround = 16;
				params.pPlayerDynamics = &playerDyn;

				playerDim.bUseCapsule = 0;
				playerDim.sizeCollider.x = pProperties->colliderRadius;
				playerDim.sizeCollider.y = 0.5f;
				playerDim.sizeCollider.z = pProperties->colliderHeight;
				// make sure that player_dimensions heightCollider > sizeCollider.z + sizeCollider.x
				const float minHeightCollider = playerDim.sizeCollider.z + playerDim.sizeCollider.x;
				if (pProperties->colliderVerticalOffset <= minHeightCollider)
				{
					playerDim.heightCollider = pProperties->colliderHeight + 0.05f;
				}
				playerDim.heightPivot = 0.0f;
				playerDim.maxUnproj = 0.0f;
				//playerDim.bUseCapsule = true;
				params.pPlayerDimensions = &playerDim;
			}
			else if (pProperties->type == ePhysicType_Static)
			{
				params.type = PE_STATIC;
			}
			else
			{
				params.type = PE_RIGID;
			}

			params.mass = pProperties->mass;
			params.density = pProperties->density;
			entity.Physicalize(params);

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
