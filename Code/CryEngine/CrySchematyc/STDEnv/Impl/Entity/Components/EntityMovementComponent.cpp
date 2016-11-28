// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityMovementComponent.h"

#include <CryPhysics/physinterface.h>
#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
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
			gEnv->pSchematyc->GetUpdateScheduler().Connect(SUpdateParams(Delegate::Make(*this, &CEntityMovementComponent::Update), m_connectionScope));
			break;
		}
	}
}

void CEntityMovementComponent::Shutdown() {}

void CEntityMovementComponent::Move(const Vec3& velolcity)
{
	m_velolcity = velolcity;
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

SGUID CEntityMovementComponent::ReflectSchematycType(CTypeInfo<CEntityMovementComponent>& typeInfo)
{
	return "04e1520c-be32-420d-9857-c39b2f7f7d4a"_schematyc_guid;
}

void CEntityMovementComponent::Register(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
	{
		auto pComponent = SCHEMATYC_MAKE_ENV_COMPONENT(CEntityMovementComponent, "Movement");
		pComponent->SetAuthor(g_szCrytek);
		pComponent->SetDescription("Entity movement component");
		pComponent->SetIcon("icons:Navigation/Move_Classic.ico");
		pComponent->SetFlags(EEnvComponentFlags::Singleton);
		scope.Register(pComponent);

		CEnvRegistrationScope componentScope = registrar.Scope(pComponent->GetGUID());
		// Functions
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityMovementComponent::Move, "07209f7e-db47-4e2b-86a8-d6ea01ccf22c"_schematyc_guid, "Move");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Move entity");
			pFunction->BindInput(1, 'vel', "Velocity", "Velocity in meters per second", Vec3(ZERO));
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityMovementComponent::SetRotation, "804855a4-a998-4fb0-9787-d2edd0b844dd"_schematyc_guid, "SetRotation");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Set entity's rotation");
			pFunction->BindInput(1, 'rot', "Rotation", "Target rotation");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityMovementComponent::Teleport, "56fcb8c0-1f1b-48b6-9ee6-1f202f969d84"_schematyc_guid, "Teleport");
			pFunction->SetAuthor(g_szCrytek);
			pFunction->SetDescription("Teleport entity");
			pFunction->BindInput(1, 'trn', "Transform", "Target transform");
			componentScope.Register(pFunction);
		}
	}
}

void CEntityMovementComponent::Update(const SUpdateContext& updateContext)
{
	if (!m_velolcity.IsZero())
	{
		IEntity& entity = EntityUtils::GetEntity(*this);
		if (IPhysicalEntity* pPhysics = entity.GetPhysics())
		{
			if (pPhysics->GetType() == PE_LIVING)
			{
				pe_action_move move;
				move.dir = m_velolcity;
				move.dt = updateContext.frameTime;
				move.iJump = 0;
				pPhysics->Action(&move);
			}
			else if (pPhysics->GetType() != PE_ARTICULATED)
			{
				pe_action_set_velocity action_set_velocity;
				action_set_velocity.v = m_velolcity;
				pPhysics->Action(&action_set_velocity);
			}
		}
		else
		{
			Matrix34 localTM = entity.GetLocalTM();
			localTM.SetTranslation(localTM.GetTranslation() + (m_velolcity * updateContext.frameTime));
			entity.SetLocalTM(localTM);
		}
		m_velolcity = Vec3(ZERO);
	}
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityMovementComponent::Register)
