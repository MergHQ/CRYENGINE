// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CoverSystemUQS.h"
#include "Cover/CoverSystem.h"

#include <CryUQS/Client/ClientIncludes.h>
#include <CryUQS/StdLib/StdLibRegistration.h>  // just to bring in some data types from the StdLib

namespace CoverSystemUQS
{

	class CGenerator_CoversAroundEntity : public UQS::Client::CGeneratorBase<CGenerator_CoversAroundEntity, CoverID>
	{
	private:

		class CCoverMonitor : public UQS::Client::IItemMonitor, public ICoverSurfaceListener
		{
		private:

			struct SCoverSurfaceIDLess
			{
				bool operator()(const CoverSurfaceID& a, const CoverSurfaceID& b) const
				{
					const uint32 id_a = a;
					const uint32 id_b = b;
					return id_a < id_b;
				}
			};

		public:

			CCoverMonitor()
				: m_bAtLeastOneMonitoredSurfaceChangedOrGotRemoved(false)
			{
				gAIEnv.pCoverSystem->RegisterCoverSurfaceListener(this);
			}

			~CCoverMonitor()
			{
				gAIEnv.pCoverSystem->UnregisterCoverSurfaceListener(this);
			}

			void AddCoverToMonitor(const CoverID& coverID)
			{
				const CoverSurfaceID surfaceID = gAIEnv.pCoverSystem->GetSurfaceID(coverID);
				m_monitoredCoverSurfaces.insert(surfaceID);
			}

			// IItemMonitor
			virtual UQS::Client::IItemMonitor::EHealthState UpdateAndCheckForCorruption(UQS::Shared::IUqsString& outExplanationInCaseOfCorruption) override
			{
				if (m_bAtLeastOneMonitoredSurfaceChangedOrGotRemoved)
				{
					outExplanationInCaseOfCorruption.Set("Item corruption occurred due to at least one of the monitored CoverSurfaces has changed or was even removed");
					return IItemMonitor::EHealthState::CorruptionOccurred;
				}
				else
				{
					return IItemMonitor::EHealthState::StillHealthy;
				}
			}
			// ~IItemMonitor

			// ICoverSurfaceListener
			void OnCoverSurfaceChangedOrRemoved(const CoverSurfaceID& affectedSurfaceID) override
			{
				if (m_monitoredCoverSurfaces.find(affectedSurfaceID) != m_monitoredCoverSurfaces.cend())
				{
					m_bAtLeastOneMonitoredSurfaceChangedOrGotRemoved = true;
				}
			}
			// ~ICoverSurfaceListener

		private:

			std::set<CoverSurfaceID, SCoverSurfaceIDLess> m_monitoredCoverSurfaces;
			bool m_bAtLeastOneMonitoredSurfaceChangedOrGotRemoved;
		};

	public:

		struct SParams
		{
			UQS::StdLib::EntityIdWrapper entityId;
			float radius;

			UQS_EXPOSE_PARAMS_BEGIN
				UQS_EXPOSE_PARAM("entityId", entityId, "ENTI", "EntitId to find covers around");
				UQS_EXPOSE_PARAM("radius", radius, "RADI", "Radius within which to find covers");
			UQS_EXPOSE_PARAMS_END
		};

		explicit CGenerator_CoversAroundEntity(const SParams& params)
			: m_params(params)
		{}

		UQS::Client::IGenerator::EUpdateStatus DoUpdate(const UQS::Client::IGenerator::SUpdateContext& updateContext, UQS::Client::CItemListProxy_Writable<CoverID>& itemListToPopulate)
		{
			std::vector<CoverID> covers;

			if (const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_params.entityId.value))
			{
				gAIEnv.pCoverSystem->GetCover(pEntity->GetPos(), m_params.radius, covers);
			}

			itemListToPopulate.CreateItemsByItemFactory(covers.size());
			std::unique_ptr<CCoverMonitor> pItemMonitorCoverChanges(new CCoverMonitor);

			for (size_t i = 0; i < covers.size(); i++)
			{
				itemListToPopulate.GetItemAtIndex(i) = covers[i];
				pItemMonitorCoverChanges->AddCoverToMonitor(covers[i]);
			}

			UQS::Core::IHubPlugin::GetHub().GetQueryManager().AddItemMonitorToQuery(updateContext.queryID, std::move(pItemMonitorCoverChanges));

			return UQS::Client::IGenerator::EUpdateStatus::FinishedGeneratingItems;
		}

	private:
		const SParams m_params;
	};

	class CFunction_CoverIDToPos3 : public UQS::Client::CFunctionBase<
		CFunction_CoverIDToPos3,
		UQS::StdLib::Pos3,
		UQS::Client::IFunctionFactory::ELeafFunctionKind::None>
	{
	public:

		struct SParams
		{
			CoverID coverId;

			UQS_EXPOSE_PARAMS_BEGIN
				UQS_EXPOSE_PARAM("coverId", coverId, "COVE", "ID of the Cover to return its position from.");
			UQS_EXPOSE_PARAMS_END
		};

	public:
		explicit CFunction_CoverIDToPos3(const UQS::Client::IFunction::SCtorContext& ctorContext)
			: CFunctionBase(ctorContext)
		{}

		UQS::StdLib::Pos3 DoExecute(const UQS::Client::IFunction::SExecuteContext& executeContext, const SParams& params) const
		{
			if (params.coverId)
			{
				return UQS::StdLib::Pos3(gAIEnv.pCoverSystem->GetCoverLocation(params.coverId));
			}
			else
			{
				executeContext.error.Set("invalid CoverID passed in");
				executeContext.bExceptionOccurred = true;
				return UQS::StdLib::Pos3(ZERO);
			}
		}
	};

	class CInstentEvaluator_TestCoverBlacklisted : public UQS::Client::CInstantEvaluatorBase<
		CInstentEvaluator_TestCoverBlacklisted,
		UQS::Client::IInstantEvaluatorFactory::ECostCategory::Cheap,
		UQS::Client::IInstantEvaluatorFactory::EEvaluationModality::Testing>
	{
	public:

		struct SParams
		{
			UQS::StdLib::EntityIdWrapper entityId;
			CoverID coverId;

			UQS_EXPOSE_PARAMS_BEGIN
				UQS_EXPOSE_PARAM("entityId", entityId, "ENTI", "EntityId of the CoverUser.");
				UQS_EXPOSE_PARAM("coverId", coverId, "COVE", "ID of the Cover to test for being blacklisted for given entity.");
			UQS_EXPOSE_PARAMS_END
		};

	public:

		UQS::Client::IInstantEvaluator::ERunStatus DoRun(const UQS::Client::IInstantEvaluator::SRunContext& runContext, const SParams& params) const
		{
			const ICoverUser* pCoverUser = gAIEnv.pCoverSystem->GetRegisteredCoverUser(params.entityId.value);

			if (!pCoverUser)
			{
				runContext.error.Set("Entity is not registered as CoverUser in the CoverSystem");
				return UQS::Client::IInstantEvaluator::ERunStatus::ExceptionOccurred;
			}

			if (!params.coverId)
			{
				runContext.error.Set("invalid CoverID provided");
				return UQS::Client::IInstantEvaluator::ERunStatus::ExceptionOccurred;
			}

			runContext.evaluationResult.bDiscardItem = !pCoverUser->IsCoverBlackListed(params.coverId);
			return UQS::Client::IInstantEvaluator::ERunStatus::Finished;
		}
	};

	class CInstantEvaluator_TestCoverOccupied : public UQS::Client::CInstantEvaluatorBase<
		CInstantEvaluator_TestCoverOccupied,
		UQS::Client::IInstantEvaluatorFactory::ECostCategory::Cheap,
		UQS::Client::IInstantEvaluatorFactory::EEvaluationModality::Testing>
	{
	public:

		struct SParams
		{
			UQS::StdLib::EntityIdWrapper entityId;
			CoverID coverId;

			UQS_EXPOSE_PARAMS_BEGIN
				UQS_EXPOSE_PARAM("entityId", entityId, "ENTI", "EntityId checking for whether the cover is already occupied by someone else.");
				UQS_EXPOSE_PARAM("coverId", coverId, "COVE", "ID of the Cover to test for being occupied by another entity.");
			UQS_EXPOSE_PARAMS_END
		};

	public:

		UQS::Client::IInstantEvaluator::ERunStatus DoRun(const UQS::Client::IInstantEvaluator::SRunContext& runContext, const SParams& params) const
		{
			if (!params.entityId.value)
			{
				runContext.error.Set("invalid EntityId provided");
				return UQS::Client::IInstantEvaluator::ERunStatus::ExceptionOccurred;
			}

			const ICoverUser* pCoverUser = gAIEnv.pCoverSystem->GetRegisteredCoverUser(params.entityId.value);

			if (!pCoverUser)
			{
				runContext.error.Format("Entity with entityId = %i is not registered as CoverUser in the CoverSystem", static_cast<int>(params.entityId.value));
				return UQS::Client::IInstantEvaluator::ERunStatus::ExceptionOccurred;
			}

			if (!params.coverId)
			{
				runContext.error.Set("invalid CoverID provided");
				return UQS::Client::IInstantEvaluator::ERunStatus::ExceptionOccurred;
			}

			const EntityId coverOccupant = gAIEnv.pCoverSystem->GetCoverOccupant(params.coverId);

			const bool bCoverIsAlreadyOccupiedBySomeoneElse =
				(coverOccupant != INVALID_ENTITYID && coverOccupant != params.entityId.value) ||
				gAIEnv.pCoverSystem->IsCoverPhysicallyOccupiedByAnyOtherCoverUser(params.coverId, *pCoverUser);

			runContext.evaluationResult.bDiscardItem = !bCoverIsAlreadyOccupiedBySomeoneElse;

			return UQS::Client::IInstantEvaluator::ERunStatus::Finished;
		}
	};

	class CInstantEvaluator_TestCoverOccludesAllEnemyEyes : public UQS::Client::CInstantEvaluatorBase<
		CInstantEvaluator_TestCoverOccludesAllEnemyEyes,
		UQS::Client::IInstantEvaluatorFactory::ECostCategory::Expensive,
		UQS::Client::IInstantEvaluatorFactory::EEvaluationModality::Testing>
	{
	public:

		struct SParams
		{
			UQS::StdLib::EntityIdWrapper entityId;
			CoverID coverId;

			UQS_EXPOSE_PARAMS_BEGIN
				UQS_EXPOSE_PARAM("entityId", entityId, "ENTI", "EntityId of the CoverUser to check for whether all enemy eyes are occluded by the cover in question.");
				UQS_EXPOSE_PARAM("coverId", coverId, "COVE", "ID of the Cover to test for occluding all enemy eyes.");
			UQS_EXPOSE_PARAMS_END
		};

	public:

		UQS::Client::IInstantEvaluator::ERunStatus DoRun(const UQS::Client::IInstantEvaluator::SRunContext& runContext, const SParams& params) const
		{
			const ICoverUser* pCoverUser = gAIEnv.pCoverSystem->GetRegisteredCoverUser(params.entityId.value);

			if (!pCoverUser)
			{
				runContext.error.Set("Entity is not registered as CoverUser in the CoverSystem");
				return UQS::Client::IInstantEvaluator::ERunStatus::ExceptionOccurred;
			}

			if (!params.coverId)
			{
				runContext.error.Set("invalid CoverID provided");
				return UQS::Client::IInstantEvaluator::ERunStatus::ExceptionOccurred;
			}

			const ICoverUser::Params& coverUserParams = pCoverUser->GetParams();

			const Vec3 coverLocation = gAIEnv.pCoverSystem->GetCoverLocation(params.coverId, coverUserParams.distanceToCover);

			const float guaranteedProtectionHeight = pCoverUser->CalculateEffectiveHeightAt(coverLocation, params.coverId);

			if (guaranteedProtectionHeight == -1.0f || (coverUserParams.minEffectiveCoverHeight > 0.0f && coverUserParams.minEffectiveCoverHeight > guaranteedProtectionHeight))
			{
				runContext.evaluationResult.bDiscardItem = true;
			}

			return UQS::Client::IInstantEvaluator::ERunStatus::Finished;
		}
	};

	void InstantiateFactories()
	{

		static const UQS::Client::CItemFactory<CoverID> itemFactory_CoverID(
			[]()
			{
				UQS::Client::CItemFactory<CoverID>::SCtorParams ctorParams;
				ctorParams.szName = "CryAISystem::CoverID";
				ctorParams.guid = "aae6748c-aa01-4139-b2b1-7118c4568b4a"_cry_guid;
				ctorParams.szDescription = "ID for referring to a specific Cover of the AI CoverSystem.";
				ctorParams.callbacks.pCreateDefaultObject = []()
				{
					return CoverID();
				};
				ctorParams.callbacks.pCreateItemDebugProxy = [](const CoverID& item, UQS::Core::IItemDebugProxyFactory& itemDebugProxyFactory)
				{
					UQS::Core::IItemDebugProxy_Sphere& sphere = itemDebugProxyFactory.CreateSphere();
					sphere.SetPosAndRadius(gAIEnv.pCoverSystem->GetCoverLocation(item), 0.1f);
				};
				ctorParams.callbacks.pAddItemToDebugRenderWorld = [](const CoverID& item, UQS::Core::IDebugRenderWorldPersistent& debugRW)
				{
					Vec3 normal;
					const Vec3 location = gAIEnv.pCoverSystem->GetCoverLocation(item, 0.0f, nullptr, &normal);
					debugRW.AddSphere(location, 0.1f, Col_White);
					debugRW.AddDirection(location, location + normal, 0.1f, 0.3f, Col_White);
				};
				return ctorParams;
			}()
		);

		static const UQS::Client::CGeneratorFactory<CGenerator_CoversAroundEntity> generator_CoversAroundEntity(
			[]()
			{
				UQS::Client::CGeneratorFactory<CGenerator_CoversAroundEntity>::SCtorParams ctorParams;
				ctorParams.szName = "CryAISystem::CoversAroundEntity";
				ctorParams.guid = "f389364a-e74c-4e73-91c2-b3653463f2bc"_cry_guid;
				ctorParams.szDescription = "Grabs IDs of all covers within a range around an entity";
				return ctorParams;
			}()
		);

		static const UQS::Client::CFunctionFactory<CFunction_CoverIDToPos3> function_CoverIDtoPos3(
			[]()
			{
				UQS::Client::CFunctionFactory<CFunction_CoverIDToPos3>::SCtorParams ctorParams;
				ctorParams.szName = "CryAISystem::CoverIDToPos3";
				ctorParams.guid = "628813b0-e6f1-41f6-b75e-2c328a61a48e"_cry_guid;
				ctorParams.szDescription = "Returns the Pos3 location of a given CoverID";
				return ctorParams;
			}()
		);

		static const UQS::Client::CInstantEvaluatorFactory<CInstentEvaluator_TestCoverBlacklisted> instantEvaluator_TestCoverBlacklisted(
			[]()
			{
				UQS::Client::CInstantEvaluatorFactory<CInstentEvaluator_TestCoverBlacklisted>::SCtorParams ctorParams;
				ctorParams.szName = "CryAISystem::TestCoverBlacklisted";
				ctorParams.guid = "cc21f0d0-694c-41fc-bec9-7a2ba052bb83"_cry_guid;
				ctorParams.szDescription = "Tests whether a cover is blacklisted for a given entity";
				return ctorParams;
			}()
		);

		static const UQS::Client::CInstantEvaluatorFactory<CInstantEvaluator_TestCoverOccupied> instantEvaluator_TestCoverOccupied(
			[]()
			{
				UQS::Client::CInstantEvaluatorFactory<CInstantEvaluator_TestCoverOccupied>::SCtorParams ctorParams;
				ctorParams.szName = "CryAISystem::TestCoverOccupied";
				ctorParams.guid = "7b987fcd-9048-429d-975b-f0a18fe8a1d2"_cry_guid;
				ctorParams.szDescription = "Tests whether a cover is already occupied logically or physically by a CoverUser other than the given one.";
				return ctorParams;
			}()
		);

		static const UQS::Client::CInstantEvaluatorFactory<CInstantEvaluator_TestCoverOccludesAllEnemyEyes> instantEvaluator_TestCoverOccludesAllEnemyEyes(
			[]()
			{
				UQS::Client::CInstantEvaluatorFactory<CInstantEvaluator_TestCoverOccludesAllEnemyEyes>::SCtorParams ctorParams;
				ctorParams.szName = "CryAISystem::TestCoverOccludesAllEnemyEyes";
				ctorParams.guid = "f0287e62-d124-4ad9-b59b-c3270dda0045"_cry_guid;
				ctorParams.szDescription = "Tests whether a cover is occluding all the enemy eyes.";
				return ctorParams;
			}()
		);

	}
}
