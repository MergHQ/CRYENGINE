#pragma once

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/IObject.h>
#include <CrySchematyc/Utils/SharedString.h>
#include <CrySchematyc/Reflection/TypeDesc.h>

#include <CryAISystem/IAISystem.h>
#include <CryAISystem/IMovementSystem.h>

#include <CryAISystem/MovementRequest.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CPathfindingComponent final
			: public IEntityComponent
			, private IMovementActorAdapter
			, private IAIPathAgent
		{
			// Dummy implementation so we can use IAISystem::CreateAndReturnNewDefaultPathFollower
			class CPathObstacles
				: public IPathObstacles
			{
				// IPathObstacles
				virtual bool IsPathIntersectingObstacles(const NavigationMeshID meshID, const Vec3& start, const Vec3& end, float radius) const override { return false; }
				virtual bool IsPointInsideObstacles(const Vec3& position) const override { return false; }
				virtual bool IsLineSegmentIntersectingObstaclesOrCloseToThem(const Lineseg& linesegToTest, float maxDistanceToConsiderClose) const override { return false; }
				// ~IPathObstacles
			};

		public:
			virtual ~CPathfindingComponent() 
			{
				Reset();
			}

			// IEntityComponent
			virtual void Initialize() override
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

			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CPathfindingComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{470C0CD6-198F-412E-A99A-15456066162B}"_cry_guid;
				return id;
			}

			void RequestMoveTo(const Vec3 &position)
			{
				CRY_ASSERT_MESSAGE(m_movementRequestId.id == 0, "RequestMoveTo can not be called while another request is being handled!");

				MovementRequest movementRequest;
				movementRequest.entityID = GetEntityId();
				movementRequest.destination = position;
				movementRequest.callback = functor(*this, &CPathfindingComponent::MovementRequestCallback);
				movementRequest.style.SetSpeed(MovementStyle::Walk);

				movementRequest.type = MovementRequest::Type::MoveTo;

				m_state = Movement::StillFinding;

				m_movementRequestId = gEnv->pAISystem->GetMovementSystem()->QueueRequest(movementRequest);
			}

			void CancelCurrentRequest()
			{
				CRY_ASSERT(m_movementRequestId.id != 0);

				gEnv->pAISystem->GetMovementSystem()->CancelRequest(m_movementRequestId);
				m_movementRequestId = 0;

				if (m_pathFinderRequestId != 0)
				{
					gEnv->pAISystem->GetMNMPathfinder()->CancelPathRequest(m_pathFinderRequestId);

					m_pathFinderRequestId = 0;
				}
			}

			bool IsProcessingRequest() const { return m_movementRequestId != 0; }

			void SetMaxAcceleration(float maxAcceleration) { m_maxAcceleration = maxAcceleration; }
			float GetMaxAcceleration() const { return m_maxAcceleration; }

			void SetMovementRecommendationCallback(std::function<void(const Vec3& recommendedVelocity)> callback) { m_movementRecommendationCallback = callback; }

		protected:
			// IMovementActorAdapter
			virtual void                  OnMovementPlanProduced() override {}

			virtual NavigationAgentTypeID GetNavigationAgentTypeID() const override { return m_navigationAgentTypeId; }
			virtual Vec3                  GetPhysicsPosition() const override { return GetEntity()->GetWorldPos(); }
			virtual Vec3                  GetVelocity() const override
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					pe_status_dynamics dynStatus;
					if (pPhysicalEntity->GetStatus(&dynStatus))
					{
						return dynStatus.v;
					}
				}

				return ZERO;
			}

			virtual Vec3                  GetMoveDirection() const override { return GetVelocity().GetNormalized(); }
			virtual Vec3                  GetAnimationBodyDirection() const override { return m_requestedTargetBodyDirection; }
			virtual EActorTargetPhase     GetActorPhase() const override { return eATP_None; }
			virtual void                  SetMovementOutputValue(const PathFollowResult& result) override;
			virtual void                  SetBodyTargetDirection(const Vec3& direction) override { m_requestedTargetBodyDirection = direction; }
			virtual void                  ResetMovementContext() override {}
			virtual void                  ClearMovementState() override;
			virtual void                  ResetBodyTarget() override {}
			virtual void                  ResetActorTargetRequest() override {}
			virtual bool                  IsMoving() const override { return !GetVelocity().IsZero(0.01f); }

			virtual void                  RequestExactPosition(const SAIActorTargetRequest* request, const bool lowerPrecision) override {}

			virtual bool                  IsClosestToUseTheSmartObject(const OffMeshLink_SmartObject& smartObjectLink) const override { return false; }
			virtual bool                  PrepareNavigateSmartObject(CSmartObject* pSmartObject, OffMeshLink_SmartObject* pSmartObjectLink) override { return false; }
			virtual void                  InvalidateSmartObjectLink(CSmartObject* pSmartObject, OffMeshLink_SmartObject* pSmartObjectLink) override {}

			virtual void                  SetInCover(const bool inCover) override {}
			virtual void                  UpdateCoverLocations() override {}
			virtual void                  InstallInLowCover(const bool inCover) override {}
			virtual void                  SetupCoverInformation() override {}
			virtual bool                  IsInCover() const override { return false; }

			virtual bool                  GetDesignedPath(SShape& pathShape) const override { return false; }
			virtual void                  CancelRequestedPath() override {}
			virtual void                  ConfigurePathfollower(const MovementStyle& style) override {}

			virtual void                  SetActorPath(const MovementStyle& style, const INavPath& navPath) override {}
			virtual void                  SetActorStyle(const MovementStyle& style, const INavPath& navPath) override {}
			virtual void                  SetStance(const MovementStyle::Stance stance) override {}

			virtual std::shared_ptr<Vec3> CreateLookTarget() override { return nullptr; }
			virtual void                  SetLookTimeOffset(float lookTimeOffset) override {}
			virtual void UpdateLooking(float updateTime, std::shared_ptr<Vec3> lookTarget, const bool targetReachable, const float pathDistanceToEnd, const Vec3& followTargetPosition, const MovementStyle& style) override {}
			// ~IMovementActorAdapter

			// IAIPathAgent
			virtual IEntity *GetPathAgentEntity() const override { return GetEntity(); }
			virtual const char *GetPathAgentName() const override { return GetEntity()->GetClass()->GetName(); }

			virtual unsigned short GetPathAgentType() const override { return AIOBJECT_ACTOR; }

			virtual float GetPathAgentPassRadius() const override { return 1.f; }
			virtual Vec3 GetPathAgentPos() const override { return GetPathAgentEntity()->GetWorldPos(); }
			virtual Vec3 GetPathAgentVelocity() const override { return GetVelocity(); }

			virtual const AgentMovementAbility& GetPathAgentMovementAbility() const override { return m_movementAbility; }

			virtual void GetPathAgentNavigationBlockers(NavigationBlockers& blockers, const PathfindRequest* pRequest) override {}

			virtual unsigned int GetPathAgentLastNavNode() const override { return 0; }
			virtual void SetPathAgentLastNavNode(unsigned int lastNavNode) override {}

			virtual void SetPathToFollow(const char* pathName) override {}
			virtual void         SetPathAttributeToFollow(bool bSpline) override {}

			virtual void SetPFBlockerRadius(int blockerType, float radius) override {}

			virtual ETriState CanTargetPointBeReached(CTargetPointRequest& request) override
			{
				request.SetResult(eTS_false);
				return eTS_false;
			}

			virtual bool UseTargetPointRequest(const CTargetPointRequest& request) override { return false; }

			virtual bool GetValidPositionNearby(const Vec3& proposedPosition, Vec3& adjustedPosition) const override { return false; }
			virtual bool GetTeleportPosition(Vec3& teleportPos) const override { return false; }

			virtual IPathFollower *GetPathFollower() const override { return m_pPathFollower.get(); }

			virtual bool IsPointValidForAgent(const Vec3& pos, uint32 flags) const override { return true; }
			// ~IAIPathAgent

			void Reset()
			{
				gEnv->pAISystem->GetMovementSystem()->UnregisterEntity(GetEntityId());
				m_pPathFollower.reset();
				SAFE_RELEASE(m_pFoundPath);
			}

			void MovementRequestCallback(const MovementRequestResult &result) {}

			INavPath *GetINavPath() { return m_pFoundPath; };
			Movement::PathfinderState GetPathfinderState() { return m_state; }
			void RequestPathTo(MNMPathRequest& request)
			{
				m_state = Movement::StillFinding;

				request.resultCallback = functor(*this, &CPathfindingComponent::OnMNMPathResult);
				request.agentTypeID = m_navigationAgentTypeId;

				m_pathFinderRequestId = gEnv->pAISystem->GetMNMPathfinder()->RequestPathTo(this, request);
			}

			void OnMNMPathResult(const MNM::QueuedPathID& requestId, MNMPathRequestResult& result)
			{
				m_pathFinderRequestId = 0;

				if (result.HasPathBeenFound())
				{
					m_state = Movement::FoundPath;

					SAFE_DELETE(m_pFoundPath);
					m_pFoundPath = result.pPath->Clone();

					// Bump version
					m_pFoundPath->SetVersion(m_pFoundPath->GetVersion() + 1);

					m_pPathFollower->Reset();
					m_pPathFollower->AttachToPath(m_pFoundPath);
				}
				else
				{
					m_state = Movement::CouldNotFindPath;
				}
			}

			NavigationAgentTypeID m_navigationAgentTypeId;
			MovementActorCallbacks m_callbacks;

			MovementRequestID m_movementRequestId = 0;
			uint32 m_pathFinderRequestId;

			std::shared_ptr<IPathFollower> m_pPathFollower;
			CPathObstacles m_pathObstacles;

			Movement::PathfinderState m_state;
			INavPath *m_pFoundPath = nullptr;

			AgentMovementAbility m_movementAbility;

			Vec3 m_requestedTargetBodyDirection;

			Schematyc::Range<0, 10000> m_maxAcceleration = 10.f;
			std::function<void(const Vec3& recommendedVelocity)> m_movementRecommendationCallback;
		};
	}
}