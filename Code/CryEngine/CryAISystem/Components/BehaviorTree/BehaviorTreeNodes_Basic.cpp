// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "BehaviorTreeNodes_Basic.h"
#include "../../Cover/CoverSystem.h"
#include <CryAISystem/BehaviorTree/Action.h>
#include <CryAISystem/MovementRequest.h>
#include <CryAISystem/IMovementSystem.h>
#include <CryUQS/Client/ClientIncludes.h>
#include <CryUQS/StdLib/StdLibRegistration.h>	// just to bring in some data types from the StdLib


// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace BehaviorTree
{

	//////////////////////////////////////////////////////////////////////////

	class Basic_MoveToPosition : public Action
	{

		typedef Action BaseClass;

	public:

		struct RuntimeData
		{
			MovementRequestID movementRequestID;
			Status pendingStatus;

			RuntimeData()
				: movementRequestID(0)
				, pendingStatus(Running)
			{}

			~RuntimeData()
			{
				if (this->movementRequestID)
				{
					gAIEnv.pMovementSystem->CancelRequest(this->movementRequestID);
					this->movementRequestID = MovementRequestID();
				}
			}

			void MovementRequestCallback(const MovementRequestResult& result)
			{
				assert(this->movementRequestID == result.requestID);

				this->movementRequestID = MovementRequestID();

				if (result == MovementRequestResult::ReachedDestination)
				{
					this->pendingStatus = Success;
				}
				else
				{
					this->pendingStatus = Failure;
				}
			}
		};

	public:

		virtual LoadResult LoadFromXml(const XmlNodeRef& xml, const LoadContext& context) override
		{
			if (BaseClass::LoadFromXml(xml, context) == LoadFailure)
			{
				return LoadFailure;
			}

			// "bbDestination" (name under which the destination position is stored in the blackboard)
			const char* szBBDestination;
			if (!xml->getAttr("bbDestination", &szBBDestination))
			{
				ErrorReporter(*this, context).LogError("Missing 'bbDestination' parameter");
				return LoadFailure;
			}
			m_destinationBBVariableId = BlackboardVariableId(szBBDestination);
			m_destinationBBVariableName = szBBDestination;

			// movementStyle
			m_movementStyle.ReadFromXml(xml);

			return LoadSuccess;
		}

		virtual void OnInitialize(const UpdateContext& context) override
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);

			Vec3 destination;
			if (context.blackboard.GetVariable(m_destinationBBVariableId, destination))
			{
				RequestMovementTo(destination, context.entityId, runtimeData);
			}
			else
			{
				ErrorReporter(*this, context).LogError("Missing entry in the blackboard or data type mismatch: '%s' (expected to be a Vec3)", m_destinationBBVariableName.c_str());
				runtimeData.pendingStatus = Failure;
			}
		}

	protected:

		virtual Status Update(const UpdateContext& context) override
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);
			return runtimeData.pendingStatus;
		}

	private:

		void RequestMovementTo(const Vec3& destination, const EntityId entityID, RuntimeData& runtimeData)
		{
			assert(!runtimeData.movementRequestID);

			MovementRequest movementRequest;
			movementRequest.entityID = entityID;
			movementRequest.destination = destination;
			movementRequest.callback = functor(runtimeData, &RuntimeData::MovementRequestCallback);
			movementRequest.style = m_movementStyle;

			runtimeData.movementRequestID = gAIEnv.pMovementSystem->QueueRequest(movementRequest);

			if (runtimeData.movementRequestID.IsInvalid())
			{
				runtimeData.pendingStatus = Failure;
			}
		}

	private:

		BlackboardVariableId m_destinationBBVariableId;
		string m_destinationBBVariableName;  // for debugging purpose
		MovementStyle m_movementStyle;
	};

	//////////////////////////////////////////////////////////////////////////

	class Basic_MoveToCover : public Action
	{
		typedef Action BaseClass;

	public:
		struct RuntimeData
		{
			Status            pendingStatus;
			MovementRequestID movementRequestID;

			RuntimeData()
				: movementRequestID(0)
				, pendingStatus(Running)
			{
			}

			~RuntimeData()
			{
				if (this->movementRequestID)
				{
					gEnv->pAISystem->GetMovementSystem()->CancelRequest(this->movementRequestID);
					this->movementRequestID = MovementRequestID();
				}
			}

			void MovementRequestCallback(const MovementRequestResult& result)
			{
				assert(this->movementRequestID == result.requestID);

				this->movementRequestID = MovementRequestID();

				if (result == MovementRequestResult::ReachedDestination)
				{
					this->pendingStatus = Success;
				}
				else
				{
					this->pendingStatus = Failure;
				}
			}
		};

		Basic_MoveToCover()
		{
		}

		virtual LoadResult LoadFromXml(const XmlNodeRef& xml, const LoadContext& context) override
		{
			if(BaseClass::LoadFromXml(xml, context) == LoadFailure)
				return LoadFailure;

			const char* szBBCoverId;
			if (!xml->getAttr("bbCoverId", &szBBCoverId))
			{
				ErrorReporter(*this, context).LogError("Missing 'bbCoverId' parameter");
				return LoadFailure;
			}
			
			m_coverIdBBVariableId = BlackboardVariableId(szBBCoverId);
			m_coverIdBBVariableName = szBBCoverId;

			m_movementStyle.ReadFromXml(xml);
			m_movementStyle.SetMovingToCover(true);

			return LoadSuccess;
		}

		virtual void OnInitialize(const UpdateContext& context) override
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);

			CoverID coverId;
			if (!context.blackboard.GetVariable<CoverID>(m_coverIdBBVariableId, coverId))
			{
				ErrorReporter(*this, context).LogError("Missing entry in the blackboard or data type mismatch: '%s' (expected to be a CoverId)", m_coverIdBBVariableName.c_str());
				runtimeData.pendingStatus = Failure;
				return;
			}

			if (!coverId)
			{
				ErrorReporter(*this, context).LogError("Invalid input Cover id (= %u).", static_cast<uint32>(coverId));
				runtimeData.pendingStatus = Failure;
				return;
			}

			ICoverUser* pCoverUser = gEnv->pAISystem->GetCoverSystem()->GetRegisteredCoverUser(context.entityId);
			if (!pCoverUser)
			{
				ErrorReporter(*this, context).LogWarning("Entity '%s' isn't registered in cover system.", context.entity.GetName());
				runtimeData.pendingStatus = Failure;
				return;
			}

			runtimeData.pendingStatus = Running;

			pCoverUser->SetNextCoverID(coverId);
			const Vec3 destPosition = pCoverUser->GetCoverLocation(coverId);

			CRY_ASSERT(!runtimeData.movementRequestID);

			MovementRequest movementRequest;
			movementRequest.entityID = context.entityId;
			movementRequest.destination = destPosition;
			movementRequest.callback = functor(runtimeData, &RuntimeData::MovementRequestCallback);
			movementRequest.style = m_movementStyle;

			runtimeData.movementRequestID = gEnv->pAISystem->GetMovementSystem()->QueueRequest(movementRequest);
		}

	private:
		virtual Status Update(const UpdateContext& context) override
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);
			return runtimeData.pendingStatus;
		}

	private:
		BlackboardVariableId m_coverIdBBVariableId;
		string          m_coverIdBBVariableName;
		MovementStyle   m_movementStyle;
	};

	//////////////////////////////////////////////////////////////////////////

	// - base class for UQS queries
	// - returns just a single item, no matter what is specified in the query blueprint
	template <class TItem>
	class QueryUQSBase : public Action
	{

		typedef Action BaseClass;

	public:

		struct RuntimeData
		{
			UQS::Client::CQueryHostT<TItem> queryHost;
		};

	public:

		virtual LoadResult LoadFromXml(const XmlNodeRef& xml, const LoadContext& context) override
		{
			if (BaseClass::LoadFromXml(xml, context) == LoadFailure)
			{
				return LoadFailure;
			}

			// "query"
			const char* szQueryBlueprintName;
			if (!xml->getAttr("query", &szQueryBlueprintName))
			{
				ErrorReporter(*this, context).LogError("Missing 'query' parameter");
				return LoadFailure;
			}

			UQS::Core::IHub* pHub = UQS::Core::IHubPlugin::GetHubPtr();

			if (!pHub)
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "QueryUQSBase: UQS plugin is not loaded.");
				return BehaviorTree::LoadFailure;
			}

			m_queryBlueprintID = pHub->GetQueryBlueprintLibrary().FindQueryBlueprintIDByName(szQueryBlueprintName);
			if (!m_queryBlueprintID.IsOrHasBeenValid())
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "QueryUQSBase: unknown query blueprint: %s", szQueryBlueprintName);
				return BehaviorTree::LoadFailure;
			}

			// "bbQueryResult" (name under which the resulting item of the query is stored in the blackboard)
			const char* szBBQueryResult;
			if (!xml->getAttr("bbQueryResult", &szBBQueryResult))
			{
				ErrorReporter(*this, context).LogError("Missing 'bbQueryResult' parameter");
				return LoadFailure;
			}
			m_queryResultBBVariableId = BlackboardVariableId(szBBQueryResult);
			m_queryResultBBVariableName = szBBQueryResult;

			return LoadSuccess;
		}

		virtual void OnInitialize(const UpdateContext& context) override
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);

			runtimeData.queryHost.SetQuerierName(context.entity.GetName());
			runtimeData.queryHost.SetQueryBlueprint(m_queryBlueprintID);

			// assume that all our queries always expect exactly 1 runtime parameter for providing the entity that executes the query and that 
			// this parameter is named "entityId"
			runtimeData.queryHost.GetRuntimeParamsStorage().AddOrReplaceFromOriginalValue("entityId", UQS::StdLib::EntityIdWrapper(context.entityId));

			runtimeData.queryHost.StartQuery();
		}

	protected:

		virtual Status Update(const UpdateContext& context) override
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);

			if (runtimeData.queryHost.IsStillRunning())
			{
				return Running;
			}

			if (runtimeData.queryHost.HasSucceeded())
			{
				if (runtimeData.queryHost.GetResultSetItemCount() > 0)
				{
					const TItem& foundItem = runtimeData.queryHost.GetResultSetItem(0).item;
					return OnFoundItem(context, foundItem, m_queryResultBBVariableId);
				}
				else
				{
					return Failure;
				}
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "QueryUQSBase: query '%s' caused a runtime exception: %s", runtimeData.queryHost.GetQueryBlueprintName(), runtimeData.queryHost.GetExceptionMessage());
				return Failure;
			}
		}

	private:

		virtual Status OnFoundItem(const BehaviorTree::UpdateContext& context, const TItem& foundItem, BlackboardVariableId& bbVariableId) = 0;

	private:

		UQS::Core::CQueryBlueprintID m_queryBlueprintID;
		BlackboardVariableId m_queryResultBBVariableId;
		string m_queryResultBBVariableName; // for debugging purpose
	};

	class Basic_QueryUQS_Pos3 : public QueryUQSBase<UQS::StdLib::Pos3>
	{
	private:
		// QueryUQSBase
		virtual Status OnFoundItem(const BehaviorTree::UpdateContext& context, const UQS::StdLib::Pos3& foundItem, BlackboardVariableId& bbVariableId) override
		{
			if (context.blackboard.SetVariable<Vec3>(bbVariableId, foundItem.value))
			{
				return Success;
			}
			else
			{
#ifdef STORE_BLACKBOARD_VARIABLE_NAMES
				ErrorReporter(*this, context).LogError("Basic_QueryUQS_Pos3: Existing entry in the blackboard mismatches the data type: key = '%s' (expected a Vec3).", bbVariableId.name.c_str());
#else
				ErrorReporter(*this, context).LogError("Basic_QueryUQS_Pos3: Existing entry in the blackboard mismatches the data type (expected a Vec3).");
#endif //~STORE_BLACKBOARD_VARIABLE_NAMES
				return Failure;
			}
		}
		// ~QueryUQSBase
	};

	class Basic_QueryUQS_CoverID : public QueryUQSBase<CoverID>
	{
	private:
		// QueryUQSBase
		virtual Status OnFoundItem(const BehaviorTree::UpdateContext& context, const CoverID& foundItem, BlackboardVariableId& bbVariableId) override
		{
			const EntityId coverOccupant = gAIEnv.pCoverSystem->GetCoverOccupant(foundItem);

			if (coverOccupant != INVALID_ENTITYID && coverOccupant != context.entityId)
			{
				// someone else occupied the cover in the meantime
				return Failure;
			}

			if (!context.blackboard.SetVariable<CoverID>(bbVariableId, foundItem))
			{
#ifdef STORE_BLACKBOARD_VARIABLE_NAMES
				ErrorReporter(*this, context).LogError("Basic_QueryUQS_CoverID: Existing entry in the blackboard mismatches the data type: key = '%s' (expected a CoverID).", bbVariableId.name.c_str());
#else
				ErrorReporter(*this, context).LogError("Basic_QueryUQS_CoverID: Existing entry in the blackboard mismatches the data type (expected a CoverID).");
#endif //~STORE_BLACKBOARD_VARIABLE_NAMES
				return Failure;
			}

			if (ICoverUser* pCoverUser = gAIEnv.pCoverSystem->GetRegisteredCoverUser(context.entityId))
			{
				pCoverUser->SetNextCoverID(foundItem);
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Basic_QueryUQS_CoverID::OnFoundItem: entity '%s' should be registered as CoverUser in the CoverSystem", context.entity.GetName());
			}

			return Success;
		}
		// ~QueryUQSBase
	};

	void RegisterBehaviorTreeNodes_Basic()
	{
		IBehaviorTreeManager& pManager = *gEnv->pAISystem->GetIBehaviorTreeManager();

		REGISTER_BEHAVIOR_TREE_NODE_WITH_NAME(pManager, Basic_MoveToPosition, "Basic_MoveToPosition");
		REGISTER_BEHAVIOR_TREE_NODE_WITH_NAME(pManager, Basic_MoveToCover, "Basic_MoveToCover");
		REGISTER_BEHAVIOR_TREE_NODE_WITH_NAME(pManager, Basic_QueryUQS_Pos3, "Basic_QueryUQS_Pos3");
		REGISTER_BEHAVIOR_TREE_NODE_WITH_NAME(pManager, Basic_QueryUQS_CoverID, "Basic_QueryUQS_CoverID");
	}

}
