// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityMovementComponent.h"

#include <CryPhysics/physinterface.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"
#include "STDModules.h"
#include "CrySerialization/yasli/BitVector.h"

namespace Schematyc
{
	SERIALIZATION_ENUM_BEGIN_NESTED(CEntityMovementComponent, eMoveModifier, "Flags")
		SERIALIZATION_ENUM(CEntityMovementComponent::eMoveModifier_None, "None", "None")
		SERIALIZATION_ENUM(CEntityMovementComponent::eMoveModifier_IgnoreX, "IgnoreX", "IgnoreX")
		SERIALIZATION_ENUM(CEntityMovementComponent::eMoveModifier_IgnoreY, "IgnoreY", "IgnoreY")
		SERIALIZATION_ENUM(CEntityMovementComponent::eMoveModifier_IgnoreZ, "IgnoreZ", "IgnoreZ")
		SERIALIZATION_ENUM(CEntityMovementComponent::eMoveModifier_IgnoreXandY, "IgnoreXandY", "IgnoreXandY")
		SERIALIZATION_ENUM(CEntityMovementComponent::eMoveModifier_IgnoreNull, "IgnoreNullComponents", "IgnoreNullComponents")
		SERIALIZATION_ENUM_END()

	void ReflectType(Schematyc::CTypeDesc<CEntityMovementComponent::eMoveModifier>& desc)
	{
		desc.SetGUID("4f9183b3-f53b-4dfa-8fcf-c54168614d4b"_schematyc_guid);
	}

	bool CEntityMovementComponent::Init()
	{
		return true;
	}

	void CEntityMovementComponent::Run(ESimulationMode simulationMode)
	{
		switch (simulationMode)
		{
		case ESimulationMode::Game:
			{
				gEnv->pSchematyc->GetUpdateScheduler().Connect(SUpdateParams(SCHEMATYC_MEMBER_DELEGATE(&CEntityMovementComponent::Update, *this), m_connectionScope));
				break;
			}
		}
	}

	void CEntityMovementComponent::Shutdown() 
	{
	}

	void CEntityMovementComponent::Move(const Vec3& velocity, eMoveModifier moveFlags)
	{
		m_velocity = velocity;
		m_moveModifier = moveFlags;
	}

	void CEntityMovementComponent::SetRotation(const CRotation& rotation)
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		if (IPhysicalEntity* pPhysics = entity.GetPhysics())
		{
			pe_params_pos ppos;
			ppos.q = rotation.ToQuat();
			pPhysics->SetParams(&ppos);
		}
		else
		{
			Matrix34 localTM = entity.GetLocalTM();
			localTM.SetRotation33(rotation.ToMatrix33());
			entity.SetLocalTM(localTM);
		}
	}

	void CEntityMovementComponent::Teleport(const CTransform& transform)
	{
		EntityUtils::GetEntity(*this).SetLocalTM(transform.ToMatrix34());
	}

	void CEntityMovementComponent::ReflectType(CTypeDesc<CEntityMovementComponent>& desc)
	{
		desc.SetGUID("04e1520c-be32-420d-9857-c39b2f7f7d4a"_schematyc_guid);
		desc.SetLabel("Movement");
		desc.SetDescription("Entity movement component");
		desc.SetIcon("icons:Navigation/Move_Classic.ico");
		desc.SetComponentFlags(EComponentFlags::Singleton);
	}

	void CEntityMovementComponent::Register(IEnvRegistrar& registrar)
	{
		CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
		{
			CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntityMovementComponent));
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityMovementComponent::Move, "07209f7e-db47-4e2b-86a8-d6ea01ccf22c"_schematyc_guid, "Move");
				pFunction->SetDescription("Move entity");
				pFunction->BindInput(1, 'vel', "Velocity", "Velocity in meters per second", Vec3(ZERO));
				pFunction->BindInput(2, 'flag', "MovementModifier", "Flag to indicate to not-set specific components of the movement-vector.", eMoveModifier_None);
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityMovementComponent::SetRotation, "804855a4-a998-4fb0-9787-d2edd0b844dd"_schematyc_guid, "SetRotation");
				pFunction->SetDescription("Set entity's rotation");
				pFunction->BindInput(1, 'rot', "Rotation", "Target rotation");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityMovementComponent::Teleport, "56fcb8c0-1f1b-48b6-9ee6-1f202f969d84"_schematyc_guid, "Teleport");
				pFunction->SetDescription("Teleport entity");
				pFunction->BindInput(1, 'trn', "Transform", "Target transform");
				componentScope.Register(pFunction);
			}
		}
	}

	void CEntityMovementComponent::Update(const SUpdateContext& updateContext)
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		if (IPhysicalEntity* pPhysics = entity.GetPhysics())
		{
			const pe_type physicType = pPhysics->GetType();
			if (physicType == PE_LIVING)
			{
				HandleModifier(pPhysics);

				pe_action_move move;
				move.dir = m_velocity;
				move.dt = updateContext.frameTime;
				move.iJump = 0;
				pPhysics->Action(&move);
			}

			if (!m_velocity.IsZero() && physicType != PE_ARTICULATED)
			{
				HandleModifier(pPhysics);

				pe_action_set_velocity action_set_velocity;
				action_set_velocity.v = m_velocity;
				pPhysics->Action(&action_set_velocity);
			}
		}
		else if (!m_velocity.IsZero())
		{
			HandleModifier(pPhysics);

			Matrix34 localTM = entity.GetLocalTM();
			localTM.SetTranslation(localTM.GetTranslation() + (m_velocity * updateContext.frameTime));
			entity.SetLocalTM(localTM);
		}
		m_velocity.zero();
	}

	void CEntityMovementComponent::HandleModifier(IPhysicalEntity* pPhysics)
	{
		if (m_moveModifier != eMoveModifier_None)
		{
			if (pPhysics)
			{
				pe_status_dynamics current;
				if (pPhysics->GetStatus(&current))
				{
					if ((m_moveModifier & eMoveModifier_IgnoreX) > 0 ||
						((m_moveModifier & eMoveModifier_IgnoreNull) && m_velocity.x == 0.0f))
						m_velocity.x = current.v.x;
					if ((m_moveModifier & eMoveModifier_IgnoreY) > 0 ||
						((m_moveModifier & eMoveModifier_IgnoreNull) && m_velocity.y == 0.0f))
						m_velocity.y = current.v.y;
					if ((m_moveModifier & eMoveModifier_IgnoreZ) > 0 ||
						((m_moveModifier & eMoveModifier_IgnoreNull) && m_velocity.z == 0.0f))
						m_velocity.z = current.v.z;
				}
			}
			else
			{
				if ((m_moveModifier & eMoveModifier_IgnoreX) > 0)
					m_velocity.x = 0.0f;
				if ((m_moveModifier & eMoveModifier_IgnoreY) > 0)
					m_velocity.y = 0.0f;
				if ((m_moveModifier & eMoveModifier_IgnoreZ) > 0)
					m_velocity.z = 0.0f;
			}
		}
	}

} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityMovementComponent::Register)
