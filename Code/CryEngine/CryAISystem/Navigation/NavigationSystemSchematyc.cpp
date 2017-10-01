// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "NavigationSystemSchematyc.h"

#include "Components/Navigation/NavigationComponent.h"

namespace NavigationSystemSchematyc
{
	bool NearestNavmeshPositionSchematyc(NavigationAgentTypeID agentTypeID, const Vec3& location, float vrange, float hrange, Vec3& meshLocation)
	{
		//TODO: GetEnclosingMeshID(agentID, location); doesn't take into account vrange and hrange and can return false when point isn't inside mesh boundary
		return gAIEnv.pNavigationSystem->GetClosestPointInNavigationMesh(agentTypeID, location, vrange, hrange, &meshLocation);
	}

	bool TestRaycastHitOnNavmeshSchematyc(NavigationAgentTypeID agentTypeID, const Vec3& startPos, const Vec3& endPos, Vec3& hitPos)
	{
		MNM::SRayHitOutput hit;
		bool bResult = gAIEnv.pNavigationSystem->NavMeshTestRaycastHit(agentTypeID, startPos, endPos, &hit);
		if (bResult)
		{
			hitPos = hit.position;
		}
		return bResult;
	}
	
	void Register(Schematyc::IEnvRegistrar& registrar, Schematyc::CEnvRegistrationScope& parentScope)
	{
		//Register Components
		CEntityAINavigationComponent::Register(registrar);

		const CryGUID NavigationSystemGUID = "ad6ac254-13b8-4a79-827c-cd6a5a8e89da"_cry_guid;

		parentScope.Register(SCHEMATYC_MAKE_ENV_MODULE(NavigationSystemGUID, "Navigation"));
		Schematyc::CEnvRegistrationScope navigationScope = registrar.Scope(NavigationSystemGUID);

		//Register Types
		navigationScope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(NavigationAgentTypeID));
		navigationScope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(CEntityAINavigationComponent::SMovementProperties));

		//NearestNavmeshPositionSchematyc
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&NearestNavmeshPositionSchematyc, "85938949-a02d-4596-9fe8-42d779eed458"_cry_guid, "NearestNavmeshPosition");
			pFunction->SetDescription("Returns the nearest position on navmesh within specified range.");
			pFunction->BindInput(1, 'agt', "AgentTypeId");
			pFunction->BindInput(2, 'loc', "Location");
			pFunction->BindInput(3, 'vr', "VerticalRange");
			pFunction->BindInput(4, 'hr', "HorizontalRange");
			pFunction->BindOutput(0, 'ret', "Found");
			pFunction->BindOutput(5, 'np', "NavmeshPosition");
			navigationScope.Register(pFunction);
		}

		//RaycastOnNavmeshSchematyc
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&TestRaycastHitOnNavmeshSchematyc, "d68f9528-ebb9-4ced-8c6e-5c9b1614ab6f"_cry_guid, "TestRaycastHitOnNavmesh");
			pFunction->SetDescription("Performs raycast hit test on navmesh and returns true if the ray hits navmesh boundaries.");
			pFunction->BindInput(1, 'agt', "AgentTypeId");
			pFunction->BindInput(2, 'sp', "StartPosition");
			pFunction->BindInput(3, 'tp', "ToPosition");
			pFunction->BindOutput(0, 'ret', "IsHit");
			pFunction->BindOutput(4, 'hp', "HitPosition");
			navigationScope.Register(pFunction);
		}
	}
}
