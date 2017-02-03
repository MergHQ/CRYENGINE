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

	void CEntityPhysicsComponent::SCollider::ReflectType(CTypeDesc<CEntityPhysicsComponent::SCollider>& desc)
	{
		desc.SetGUID("80b27999-22f4-475f-b3c3-a56e2767a23b"_schematyc_guid);
	}

	// N.B. Non-intrusive serialization is used here only to ensure backward compatibility.
	//      If files were patched we could instead reflect members and let the system serialize them automatically.
	inline bool Serialize(Serialization::IArchive& archive, CEntityPhysicsComponent::SCollider& value, const char* szName, const char* szLabel)
	{
		archive(value.type, "type", "Type");
		if (value.type == CEntityPhysicsComponent::ePhysicType_Living)
		{
			if (archive.openBlock("collider", "Collision Proxy"))
			{
				archive(value.height, "height", "Height");
				archive(value.radius, "radius", "Radius");
				archive(value.verticalOffset, "verticalOffset", "VerticalOffset");
				archive(value.gravity, "gravity", "Gravity");
				archive(Serialization::Range(value.friction, 0.01f, 1.0f), "friction", "Friction");
				archive.closeBlock();
			}
		}
		return true;
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
		EntityUtils::GetEntityObject(*this).GetGeomUpdateSignal().GetSlots().Connect(SCHEMATYC_MEMBER_DELEGATE(&CEntityPhysicsComponent::OnGeometryChanged, *this), m_connectionScope);
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
			CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntityPhysicsComponent));
			// Functions
			{				
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityPhysicsComponent::SetEnabled, "4DC3B15A-D143-4161-8C1F-F6FEE627A85A"_schematyc_guid, "SetPhysicsEnabled");
				pFunction->SetDescription("Enable/disable physics (without destroying/recreating)");
				pFunction->SetFlags(EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'ena', "Enabled");
				componentScope.Register(pFunction);
			}

			//// Signals  #TODO
			//{
			//	auto pSignal = SCHEMATYC_MAKE_ENV_SIGNAL_TYPE(SPhysicCollisionSignal, "PhysicCollisionSignal");
			//	pSignal->SetDescription("Sent when two physical objects collided");
			//	componentScope.Register(pSignal);
			//}
		}
	}

	void CEntityPhysicsComponent::ReflectType(CTypeDesc<CEntityPhysicsComponent>& desc)
	{
		desc.SetGUID("178287AF-CFD5-4757-93FA-2CDE64FB633C"_schematyc_guid);
		desc.SetLabel("Physics");
		desc.SetDescription("Entity physics component");
		desc.SetIcon("icons:schematyc/entity_physics_component.ico");
		desc.AddMember(&CEntityPhysicsComponent::m_mass, 'mass', "mass", "Mass", nullptr, -1.0f);
		desc.AddMember(&CEntityPhysicsComponent::m_density, 'dens', "density", "Density", nullptr, -1.0f);
		desc.AddMember(&CEntityPhysicsComponent::m_bEnabled, 'ena', "enabled", "Enabled", nullptr, true);
		desc.AddMember(&CEntityPhysicsComponent::m_collider, 'call', "properties", "Properties", nullptr);
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
			SEntityPhysicalizeParams params;
			pe_player_dimensions playerDim;
			pe_player_dynamics playerDyn;

			if (m_collider.type == ePhysicType_Living)
			{
				params.type = PE_LIVING;
				params.mass = m_mass;
				params.density = m_density;

				IPhysicalEntity* pPhysicalEntity = entity.GetPhysics();
				if (pPhysicalEntity)
				{
					pPhysicalEntity->GetParams(&playerDyn);
				}
				playerDyn.gravity = Vec3(0, 0, -m_collider.gravity);
				playerDyn.kAirControl = m_collider.friction;
				playerDyn.kAirResistance = 1.0f;
				playerDyn.mass = m_mass;
				playerDyn.minSlideAngle = 45;
				playerDyn.maxClimbAngle = 40;
				playerDyn.minFallAngle = 50;
				playerDyn.maxVelGround = 16;
				params.pPlayerDynamics = &playerDyn;

				playerDim.bUseCapsule = 0;
				playerDim.sizeCollider.x = m_collider.radius;
				playerDim.sizeCollider.y = 0.5f;
				playerDim.sizeCollider.z = m_collider.height;
				// make sure that player_dimensions heightCollider > sizeCollider.z + sizeCollider.x
				const float minHeightCollider = playerDim.sizeCollider.z + playerDim.sizeCollider.x;
				if (m_collider.verticalOffset <= minHeightCollider)
				{
					playerDim.heightCollider = m_collider.height + 0.05f;
				}
				playerDim.heightPivot = 0.0f;
				playerDim.maxUnproj = 0.0f;
				//playerDim.bUseCapsule = true;
				params.pPlayerDimensions = &playerDim;
			}
			else if (m_collider.type == ePhysicType_Static)
			{
				params.type = PE_STATIC;
			}
			else
			{
				params.type = PE_RIGID;
			}

			params.mass = m_mass;
			params.density = m_density;
			entity.Physicalize(params);

			SetEnabled(m_bEnabled);
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
