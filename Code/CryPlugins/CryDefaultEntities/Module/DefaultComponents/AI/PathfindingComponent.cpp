#include "StdAfx.h"
#include "PathfindingComponent.h"

namespace Cry
{
	namespace DefaultComponents
	{
		static void RegisterPathfindingComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CPathfindingComponent));
				// Functions
			}
		}

		void CPathfindingComponent::ReflectType(Schematyc::CTypeDesc<CPathfindingComponent>& desc)
		{
			desc.SetGUID(CPathfindingComponent::IID());
			desc.SetEditorCategory("AI");
			desc.SetLabel("Pathfinder");
			desc.SetDescription("Exposes the ability to get path finding callbacks");
			//desc.SetIcon("icons:ObjectTypes/object.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CPathfindingComponent::m_maxAcceleration, 'maxa', "MaxAcceleration", "Maximum Acceleration", "Maximum possible physical acceleration", 10.f);
		}

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
			desc.AddMember(&SOnMovementRecommendation::m_velocity, 'vel', "Velocity", "Velocity", "The velocity proposed by the path finding solver");
		}

		void CPathfindingComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			Initialize();
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