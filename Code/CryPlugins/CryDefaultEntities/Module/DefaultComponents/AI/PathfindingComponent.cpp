#include "StdAfx.h"
#include "PathfindingComponent.h"

namespace Cry
{
namespace DefaultComponents
{

struct SOnMovementRecommendation
{
	SOnMovementRecommendation() {}
	SOnMovementRecommendation(const Vec3& velocity)
		: m_velocity(velocity) {}

	Vec3 m_velocity;
};

static void ReflectType(Schematyc::CTypeDesc<SOnMovementRecommendation>& desc)
{
	desc.SetGUID("{A2C579DA-37FC-4622-9EA0-95971DE23BE1}"_cry_guid);
	desc.SetLabel("On Movement Recommendation");
	desc.AddMember(&SOnMovementRecommendation::m_velocity, 'vel', "Velocity", "Velocity", "The velocity proposed by the path finding solver", Vec3(0.0f));
}

void CPathfindingComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SOnMovementRecommendation));
}

void CPathfindingComponent::ReflectType(Schematyc::CTypeDesc<CPathfindingComponent>& desc)
{
	desc.SetGUID(CPathfindingComponent::IID());
	desc.SetEditorCategory("AI");
	desc.SetLabel("Pathfinder");
	desc.SetDescription("Exposes the ability to get path finding callbacks");
	//desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	desc.AddMember(&CPathfindingComponent::m_maxAcceleration, 'maxa', "MaxAcceleration", "Maximum Acceleration", "Maximum possible physical acceleration", 10.f);
}

void CPathfindingComponent::Initialize()
{
	Reset();

	m_navigationAgentTypeId = gEnv->pAISystem->GetNavigationSystem()->GetAgentTypeID("MediumSizedCharacters");

	m_callbacks.queuePathRequestFunction = functor(*this, &CPathfindingComponent::RequestPathTo);
	m_callbacks.checkOnPathfinderStateFunction = functor(*this, &CPathfindingComponent::GetPathfinderState);
	m_callbacks.getPathFollowerFunction = functor(*this, &CPathfindingComponent::GetPathFollower);
	m_callbacks.getPathFunction = functor(*this, &CPathfindingComponent::GetINavPath);

	gEnv->pAISystem->GetMovementSystem()->RegisterEntity(GetEntityId(), m_callbacks, *this);

	if (m_pPathFollower == nullptr)
	{
		PathFollowerParams params;
		params.maxAccel = m_maxAcceleration;
		params.maxSpeed = params.maxAccel;
		params.minSpeed = 0.f;
		params.normalSpeed = params.maxSpeed;

		params.use2D = false;

		m_pPathFollower = gEnv->pAISystem->CreateAndReturnNewDefaultPathFollower(params, m_pathObstacles);
	}

	m_movementAbility.b3DMove = true;
}

void CPathfindingComponent::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_START_GAME:
		case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
		{
			Initialize();
		}
		break;
	}
}

uint64 CPathfindingComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_START_GAME) | BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
}

void CPathfindingComponent::SetMovementOutputValue(const PathFollowResult& result)
{
	if (Schematyc::IObject* pSchematycObject = m_pEntity->GetSchematycObject())
	{
		pSchematycObject->ProcessSignal(SOnMovementRecommendation(result.velocityOut), GetGUID());
	}

	if (m_movementRecommendationCallback)
	{
		m_movementRecommendationCallback(result.velocityOut);
	}
}

void CPathfindingComponent::ClearMovementState()
{
	if (Schematyc::IObject* pSchematycObject = m_pEntity->GetSchematycObject())
	{
		pSchematycObject->ProcessSignal(SOnMovementRecommendation(ZERO), GetGUID());
	}

	if (m_movementRecommendationCallback)
	{
		m_movementRecommendationCallback(ZERO);
	}
}
}
}
