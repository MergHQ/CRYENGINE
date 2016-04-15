// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Area.h"
#include "AreaManager.h"

//////////////////////////////////////////////////////////////////////////
CAreaManager::CAreaManager(CEntitySystem* pEntitySystem)  // : CSoundAreaManager(pEntitySystem)
	: m_pEntitySystem(pEntitySystem)
	, m_bAreasDirty(true)
{
	// Minimize run-time allocations.
	m_mapEntitiesToUpdate.reserve(32);

	if (ISystemEventDispatcher* pSystemEventDispatcher = gEnv->pSystem->GetISystemEventDispatcher())
	{
		pSystemEventDispatcher->RegisterListener(this);
	}
}

//////////////////////////////////////////////////////////////////////////
CAreaManager::~CAreaManager()
{
	CRY_ASSERT(m_apAreas.size() == 0);

	if (ISystemEventDispatcher* pSystemEventDispatcher = gEnv->pSystem->GetISystemEventDispatcher())
	{
		pSystemEventDispatcher->RemoveListener(this);
	}
}

//////////////////////////////////////////////////////////////////////////
CArea* CAreaManager::CreateArea()
{
	CArea* pArea = new CArea(this);

	m_apAreas.push_back(pArea);

	m_bAreasDirty = true;

	return pArea;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::Unregister(CArea const* const pArea)
{
	// Remove the area reference from the entity's area cache.
	m_mapAreaCache.erase_if(SRemoveIfNoAreasLeft(pArea, m_apAreas, m_apAreas.size()));

	// Also remove the area reference itself.
	TAreaPointers::iterator IterAreas(m_apAreas.begin());
	TAreaPointers::const_iterator const IterAreasEnd(m_apAreas.end());

	for (; IterAreas != IterAreasEnd; ++IterAreas)
	{
		if (pArea == (*IterAreas))
		{
			m_apAreas.erase(IterAreas);

			break;
		}
	}

	if (m_apAreas.empty())
	{
		stl::free_container(m_apAreas);
	}

	m_bAreasDirty = true;
}

//////////////////////////////////////////////////////////////////////////
IArea const* const CAreaManager::GetArea(size_t const nAreaIndex) const
{
#if defined(DEBUG_AREAMANAGER)
	if (nAreaIndex >= m_apAreas.size())
	{
		CryFatalError("<AreaManager>: GetArea index out of bounds (Count: %d Index: %d)!", static_cast<int>(m_apAreas.size()), static_cast<int>(nAreaIndex));
	}
#endif // DEBUG_AREAMANAGER

	return static_cast<IArea*>(m_apAreas.at(nAreaIndex));
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::DrawLinkedAreas(EntityId linkedId) const
{
	std::vector<CArea*> areas;
	size_t const nNumAreas = GetLinkedAreas(linkedId, -1, areas);

	for (size_t iIdx = 0; iIdx < nNumAreas; ++iIdx)
	{
		areas[iIdx]->Draw(iIdx);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAreaManager::GetLinkedAreas(EntityId linkedId, EntityId* pOutArray, int& outAndMaxResults) const
{
	int nMaxResults = outAndMaxResults;
	int nArrayIndex = 0;
	size_t const nCount = m_apAreas.size();

	for (size_t aIdx = 0; aIdx < nCount; aIdx++)
	{
		if (CArea* pArea = m_apAreas[aIdx])
		{
			const std::vector<EntityId>& ids = *pArea->GetEntities();

			if (!ids.empty())
			{
				size_t const nidCount = ids.size();

				for (size_t eIdx = 0; eIdx < nidCount; eIdx++)
				{
					if (ids[eIdx] == linkedId)
					{
						if (nArrayIndex < nMaxResults)
						{
							EntityId areaId = pArea->GetEntityID();
							pOutArray[nArrayIndex] = areaId;
							nArrayIndex++;
						}
						else
						{
							outAndMaxResults = nArrayIndex;
							return false;
						}
					}
				}
			}
		}
	}

	outAndMaxResults = nArrayIndex;
	return true;
}

//////////////////////////////////////////////////////////////////////////
size_t CAreaManager::GetLinkedAreas(EntityId linkedId, int areaId, std::vector<CArea*>& areas) const
{
	size_t const nCount = m_apAreas.size();

	for (size_t aIdx = 0; aIdx < nCount; aIdx++)
	{
		if (CArea* pArea = m_apAreas[aIdx])
		{
			const std::vector<EntityId>& ids = *pArea->GetEntities();

			if (!ids.empty())
			{
				size_t const nidCount = ids.size();

				for (size_t eIdx = 0; eIdx < nidCount; eIdx++)
				{
					if (ids[eIdx] == linkedId)
					{
						if (areaId == -1 || areaId == pArea->GetID())
							areas.push_back(pArea);
					}
				}
			}
		}
	}

	return areas.size();
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::MarkEntityForUpdate(EntityId const nEntityID)
{
	const size_t framesToUpdate = 1;
	TEntitiesToUpdateMap::iterator const iter(m_mapEntitiesToUpdate.find(nEntityID));

	if (iter != m_mapEntitiesToUpdate.end())
	{
		iter->second = framesToUpdate;
	}
	else
	{
		m_mapEntitiesToUpdate.insert(std::make_pair(nEntityID, framesToUpdate));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::TriggerAudioListenerUpdate(IArea const* const _pArea)
{
	CArea const* const pArea = static_cast<CArea const* const>(_pArea);
	IEntityItPtr const pIt = m_pEntitySystem->GetEntityIterator();

	while (!pIt->IsEnd())
	{
		IEntity const* const pEntity = pIt->Next();

		// Do this for all audio listener entities.
		if (pEntity != nullptr && (pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)
		{
			ExitArea(pEntity, pArea);
			MarkEntityForUpdate(pEntity->GetId());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::Update()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

	// Update the area grid data structure.
	UpdateDirtyAreas();

	if (!m_mapEntitiesToUpdate.empty())
	{
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
		float const debugPosX = 650.0f;
		float debugPosY = 500.0f;
		float const fColor[4] = { 0.0f, 1.0f, 0.0f, 0.7f };
		bool const bDrawDebug = CVar::pDrawAreaDebug->GetIVal() != 0;

		if (bDrawDebug)
		{
			gEnv->pRenderer->Draw2dLabel(debugPosX, debugPosY, 1.35f, fColor, false, "Entities to update: %d\n", static_cast<int>(m_mapEntitiesToUpdate.size()));
			debugPosY += 12.0f;
		}
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

		// We need to swap here as UpdateEntity could recursively
		// add to the container that we are iterating over.
		TEntitiesToUpdateMap mapEntitiesToUpdate;
		mapEntitiesToUpdate.swap(m_mapEntitiesToUpdate);

		for (auto& entityIdPair : mapEntitiesToUpdate)
		{
			CRY_ASSERT(entityIdPair.second > 0);
			--(entityIdPair.second);

			IEntity const* const pIEntity = g_pIEntitySystem->GetEntity(entityIdPair.first);

			if (pIEntity != nullptr)
			{
				// Add a Z offset of at least 0.11 to be slightly above the offset of 0.1 set through "CShapeObject::GetShapeZOffset".
				Vec3 const position(pIEntity->GetWorldPos() + Vec3(0.0f, 0.0f, 0.11f));
				UpdateEntity(position, pIEntity);

#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
				if (bDrawDebug)
				{
					gEnv->pRenderer->Draw2dLabel(debugPosX + 10.0f, debugPosY, 1.35f, fColor, false, "Entity: %d (%s) Pos: (%.2f, %.2f, %.2f)\n", entityIdPair.first, pIEntity ? pIEntity->GetName() : "nullptr", position.x, position.y, position.z);
					debugPosY += 12.0f;
				}
#endif  // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
			}
		}

		mapEntitiesToUpdate.erase_if(IsDoneUpdating);
		m_mapEntitiesToUpdate.insert(mapEntitiesToUpdate.begin(), mapEntitiesToUpdate.end());
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::UpdateEntity(Vec3 const& rPos, IEntity const* const pEntity)
{
	EntityId const nEntityID = pEntity->GetId();
	SAreasCache* pAreaCache = GetAreaCache(nEntityID);

	// Create a new area cache if necessary.
	if (pAreaCache == nullptr)
	{
		pAreaCache = MakeAreaCache(nEntityID);
	}

	CRY_ASSERT(pAreaCache != nullptr);

	// Audio listeners and moving entities affected by environment changes need to update more often
	// to ensure smooth fading.
	uint32 const nExtendedFlags = pEntity->GetFlagsExtended();
	float const fPosDelta =
	  ((nExtendedFlags & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0) || ((nExtendedFlags & ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE) != 0)
	  ? 0.01f
	  : CVar::es_EntityUpdatePosDelta;

	if (pAreaCache != nullptr && !rPos.IsEquivalent(pAreaCache->vLastUpdatePos, fPosDelta))
	{
		pAreaCache->vLastUpdatePos = rPos;

		// First mark all cache entries that as if they are not in the grid.
		for (SAreaCacheEntry& areaCacheEntry : pAreaCache->aoAreas)
		{
			CArea* const pArea = areaCacheEntry.pArea;

#if defined(DEBUG_AREAMANAGER)
			CheckArea(pArea);
#endif // DEBUG_AREAMANAGER

			areaCacheEntry.bInGrid = false;

			// Now pre-calculate position data.
			pArea->InvalidateCachedAreaData(nEntityID);
			pArea->CalcPosType(nEntityID, rPos);
		}

		TAreaPointers const& areasAtPos(m_areaGrid.GetAreas(rPos));

		for (CArea* const pArea : areasAtPos)
		{
			// Mark cache entries as if they are in the grid.
			SAreaCacheEntry* pAreaCacheEntry = nullptr;

			if (pAreaCache->GetCacheEntry(pArea, &pAreaCacheEntry))
			{
				// cppcheck-suppress nullPointer
				pAreaCacheEntry->bInGrid = true;
			}
			else
			{
				// if they are not yet in the cache, add them
				pAreaCache->aoAreas.push_back(SAreaCacheEntry(pArea, false, false));
				pArea->OnAddedToAreaCache(pEntity);
			}

#if defined(DEBUG_AREAMANAGER)
			CheckArea(pArea);
#endif // DEBUG_AREAMANAGER
		}

		// Go through all cache entries and process the areas.
		for (SAreaCacheEntry& areaCacheEntry : pAreaCache->aoAreas)
		{
			CArea* const pArea = areaCacheEntry.pArea;

#if defined(DEBUG_AREAMANAGER)
			CheckArea(pArea);
#endif // DEBUG_AREAMANAGER

			// check if Area is hidden
			IEntity const* const pAreaEntity = m_pEntitySystem->GetEntity(pArea->GetEntityID());
			bool bIsHidden = (pAreaEntity && pAreaEntity->IsHidden());

			// area was just hidden
			if (bIsHidden && pArea->IsActive())
			{
				pArea->LeaveArea(pEntity);
				pArea->LeaveNearArea(pEntity);
				areaCacheEntry.bNear = false;
				areaCacheEntry.bInside = false;
				pArea->SetActive(false);
				continue;
			}

			// area was just unhidden
			if (!bIsHidden && !pArea->IsActive())
			{
				// ProcessArea will take care of properly setting cache entry data.
				areaCacheEntry.bNear = false;
				areaCacheEntry.bInside = false;
				pArea->SetActive(true);
			}

			// We process only for active areas in which grid we are.
			// Areas in our cache in which grid we are not get removed down below anyhow.
			if (pArea->IsActive())
			{
				ProcessArea(pArea, areaCacheEntry, pAreaCache, rPos, pEntity);
			}
		}

		// Go through all areas again and send accumulated events. (needs to be done in a separate step)
		for (SAreaCacheEntry& areaCacheEntry : pAreaCache->aoAreas)
		{
#if defined(DEBUG_AREAMANAGER)
			CheckArea(areaCacheEntry.pArea);
#endif // DEBUG_AREAMANAGER

			areaCacheEntry.pArea->SendCachedEventsFor(nEntityID);
		}

		// Remove all entries in the cache which are no longer in the grid.
		if (!pAreaCache->aoAreas.empty())
		{
			pAreaCache->aoAreas.erase(std::remove_if(pAreaCache->aoAreas.begin(), pAreaCache->aoAreas.end(), SIsNotInGrid(pEntity, m_apAreas, m_apAreas.size())), pAreaCache->aoAreas.end());
		}

		if (pAreaCache->aoAreas.empty())
		{
			DeleteAreaCache(nEntityID);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAreaManager::QueryAudioAreas(Vec3 const& pos, SAudioAreaInfo* const pResults, size_t const numMaxResults, size_t& outNumResults)
{
	outNumResults = 0;

	if (pResults != nullptr && numMaxResults > 0)
	{
		// Make sure the area grid is recompiled, if needed, before accessing it
		UpdateDirtyAreas();

		uint32 numAreas = 0;
		TAreaPointers const& areasAtPos(m_areaGrid.GetAreas(pos));
		SAreasCache areaCache;

		for (CArea* const pArea : areasAtPos)
		{
#if defined(DEBUG_AREAMANAGER)
			CheckArea(pArea);
#endif // DEBUG_AREAMANAGER

			SAreaCacheEntry areaCacheEntry(pArea, false, false);
			areaCache.aoAreas.push_back(areaCacheEntry);
		}

		for (SAreaCacheEntry& areaCacheEntry : areaCache.aoAreas)
		{
			CArea* const pArea = areaCacheEntry.pArea;
			IEntity const* const pAreaEntity = m_pEntitySystem->GetEntity(pArea->GetEntityID());

			if (pAreaEntity && !pAreaEntity->IsHidden())
			{
				size_t const attachedEntities = pArea->GetEntityAmount();

				if (attachedEntities > 0)
				{
					for (size_t i = 0; i < attachedEntities; ++i)
					{
						IEntity const* const pEntity = gEnv->pEntitySystem->GetEntity(pArea->GetEntityByIdx(i));

						if (pEntity != nullptr)
						{
							IEntityAudioProxy const* const pIEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));

							if (pIEntityAudioProxy != nullptr)
							{
								AudioEnvironmentId const nEnvironmentID = pIEntityAudioProxy->GetEnvironmentID();
								float const fEnvironmentFadeDistance = pIEntityAudioProxy->GetEnvironmentFadeDistance();

								if (nEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID)
								{
									float environmentAmount = 0.0f;
									GetEnvFadeValue(areaCache, areaCacheEntry, pos, environmentAmount);

									if (environmentAmount > 0.0f)
									{
										if (outNumResults < numMaxResults)
										{
											pResults[outNumResults].pArea = pArea;
											pResults[outNumResults].amount = environmentAmount;
											pResults[outNumResults].audioEnvironmentId = nEnvironmentID;
											pResults[outNumResults].envProvidingEntityId = pEntity->GetId();
											++outNumResults;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return outNumResults > 0;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::ProcessArea(CArea* const pArea, SAreaCacheEntry& areaCacheEntry, SAreasCache* const pAreaCache, Vec3 const& pos, IEntity const* const pIEntity)
{
	Vec3 Closest3d;
	bool bExclusiveUpdate = false;
	EntityId const nEntityID = pIEntity->GetId();
	bool const bIsPointWithin = (pArea->CalcPosType(nEntityID, pos) == AREA_POS_TYPE_2DINSIDE_ZINSIDE);

	if (bIsPointWithin)
	{
		// Was far/near but is inside now.
		if (!areaCacheEntry.bInside)
		{
			// We're inside now and not near anymore.
			pArea->EnterArea(pIEntity);
			areaCacheEntry.bInside = true;
			areaCacheEntry.bNear = false;

			// Notify possible lower priority areas about this event.
			NotifyAreas(pArea, pAreaCache, pIEntity);
		}

		uint32 const nEntityFlagsExtended = pIEntity->GetFlagsExtended();

		if ((nEntityFlagsExtended & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0 || (nEntityFlagsExtended & ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE) > 0)
		{
			// This updates the distance to closest border when inside the area. (TODO: Can this be called once from within CalcPosType when position is within area?)
			pArea->CalcPointWithinDist(nEntityID, pos, false);

			bExclusiveUpdate = ProceedExclusiveUpdateByHigherArea(pAreaCache, pIEntity, pos, pArea, pos); // TODO: Double check why rPos is passed twice!

			if (!bExclusiveUpdate)
			{
				if (areaCacheEntry.bInside)
				{
					SEntityEvent event(nEntityID, 0, 0, 0, 1.0f, 1.0f, 1.0f, pos);
					event.event = ENTITY_EVENT_MOVEINSIDEAREA;
					pArea->AddCachedEvent(event);
				}
			}
		}
	}
	else
	{
		// This is optimized internally and might not recalculate but rather retrieve the cached data.
		float const distanceSq = pArea->CalcPointNearDistSq(nEntityID, pos, Closest3d, false);
		float const greatestFadeDistance = pArea->GetGreatestFadeDistance();
		float const greatestEnvFadeDistance = pArea->GetGreatestEnvironmentFadeDistance();
		bool const isNear = ((distanceSq > 0.0f) && (distanceSq < greatestFadeDistance * greatestFadeDistance));

		// Was near or inside but is either far or hidden now.
		if (areaCacheEntry.bInside)
		{
			pArea->LeaveArea(pIEntity);
			areaCacheEntry.bInside = false;

			// Needs to be temporarily near again.
			areaCacheEntry.bNear = true;

			// Notify possible lower priority areas about this event.
			NotifyAreas(pArea, pAreaCache, pIEntity);
		}

		if (isNear)
		{
			if (!areaCacheEntry.bNear)
			{
				pArea->EnterNearArea(pIEntity, Closest3d);
				areaCacheEntry.bNear = true;
			}

			// is near now
			bExclusiveUpdate = ProceedExclusiveUpdateByHigherArea(pAreaCache, pIEntity, pos, pArea, Closest3d);

			// if there is no cached event waiting, Fade can be overwritten
			if (!bExclusiveUpdate)
			{
				float const distance = sqrt_tpl(distanceSq);
				float const fade = (greatestFadeDistance > 0.0f) ? max(0.0f, (greatestFadeDistance - distance) / greatestFadeDistance) : 0.0f;
				float const environmentFade = (greatestEnvFadeDistance > 0.0f) ? max(0.0f, (greatestEnvFadeDistance - distance) / greatestEnvFadeDistance) : 0.0f;

				CRY_ASSERT(areaCacheEntry.bNear && !areaCacheEntry.bInside); // We must be near but not inside yet!
				SEntityEvent event(nEntityID, 0, 0, 0, fade, distanceSq, environmentFade, Closest3d);
				event.event = ENTITY_EVENT_MOVENEARAREA;
				pArea->AddCachedEvent(event);
			}
		}
		else if (areaCacheEntry.bNear)
		{
			pArea->LeaveNearArea(pIEntity);
			areaCacheEntry.bNear = false;
		}
	}
}

//	checks for higher areas in the same group, the player is also in
//	finds multiple point candidates and picks the furthers
//	change fade value the deeper player is inside
//////////////////////////////////////////////////////////////////////////
bool CAreaManager::ProceedExclusiveUpdateByHigherArea(SAreasCache* const pAreaCache, IEntity const* const pEntity, Vec3 const& rEntityPos, CArea* const pArea, Vec3 const& vOnLowerHull)
{
	// we try to catch 4 cases here.
	// 1) not-inside-low, not-inside-high: both areas a close, it depends on which area is closer to the player
	// 2) inside-low, not-inside-high: typical approaching high area scenario
	// 3) not-inside-low, inside-high: reversed approach from within a high area to fade in a low area
	// 4) inside-low, inside-high: both inside, so we fade in the lower area on the higher area hull, if that point is also in the lower

	EntityId const entityId = pEntity->GetId();
	bool bResult = false;
	int const currentGroupId = pArea->GetGroup();

	if (currentGroupId > 0)
	{
		TAreaPointers apAreasOfSameGroup;

		//	Find areas of higher priority that belong to the same group as the passed pArea.
		int const minPriority = pArea->GetPriority();

		for (SAreaCacheEntry const& areaCacheEntry : pAreaCache->aoAreas)
		{
			CArea* const pHigherPrioArea = areaCacheEntry.pArea;

#if defined(DEBUG_AREAMANAGER)
			CheckArea(pHigherPrioArea);
#endif // DEBUG_AREAMANAGER

			// Must be of same group.
			if (currentGroupId == pHigherPrioArea->GetGroup())
			{
				// Only check areas which are active (not hidden).
				if (minPriority < pHigherPrioArea->GetPriority() && pHigherPrioArea->IsActive())
				{
					apAreasOfSameGroup.push_back(pHigherPrioArea);
				}
			}
		}

		// Continue only if there were areas found of higher priority.
		if (!apAreasOfSameGroup.empty())
		{
			bool bPosInLowerArea = false;
			SAreaCacheEntry* pAreaCachEntry = nullptr;

			if (pAreaCache->GetCacheEntry(pArea, &pAreaCachEntry))
			{
				// cppcheck-suppress nullPointer
				bPosInLowerArea = pAreaCachEntry->bInside;
			}

			float fLargestDistanceSq = bPosInLowerArea ? 0.0f : FLT_MAX;
			CArea* pHigherAreaWithLargestDistance = nullptr;
			float fHigherGreatestFadeDistance = 0.0f;
			float fHigherGreatestEnvFadeDistance = 0.0f;
			Vec3 vHigherClosest3d(ZERO);
			bool bPosInHighestArea = false;

			TAreaPointers::const_iterator const IterAreaPointersEnd(apAreasOfSameGroup.end());

			for (TAreaPointers::const_iterator IterAreaPointers(apAreasOfSameGroup.begin()); IterAreaPointers != IterAreaPointersEnd; ++IterAreaPointers)
			{
				CArea* const pHigherArea = (*IterAreaPointers);
				Vec3 vClosest3d(ZERO);

				// This is optimized internally and might not recalculate but rather retrieve the cached data.
				bool const bPosInHighArea = pHigherArea->CalcPointWithin(entityId, rEntityPos);
				float const fDistanceSq = pHigherArea->CalcPointNearDistSq(entityId, rEntityPos, vClosest3d, false);

				bool bUseThisArea = fDistanceSq > 0.0f && ((fLargestDistanceSq < fDistanceSq && bPosInLowerArea) || (fDistanceSq < fLargestDistanceSq && !bPosInLowerArea));
				// reject cases when Pos is not inside new High Area and we already found a high area
				// and add the case where Pos is inside new High Area, but would be rejected by Higher Area,
				bUseThisArea = (bUseThisArea && !(!bPosInHighArea && bPosInHighestArea)) || (bPosInHighArea && !bPosInHighestArea);

				if (bUseThisArea)
				{
					fLargestDistanceSq = fDistanceSq;
					pHigherAreaWithLargestDistance = pHigherArea;
					fHigherGreatestFadeDistance = pHigherArea->GetGreatestFadeDistance();
					fHigherGreatestEnvFadeDistance = pHigherArea->GetGreatestEnvironmentFadeDistance();
					vHigherClosest3d = vClosest3d;
					bPosInHighestArea = bPosInHighArea;
				}
			}

			// case 2) if we are in the low area, but not inside the high one, then no need to update exclusive
			if (!bPosInLowerArea || bPosInHighestArea)
			{
				// did we find the best Higher Area to control this Area?
				if (pHigherAreaWithLargestDistance != nullptr)
				{
					bool const bHighest3dPointInLowerArea = pArea->CalcPointWithin(INVALID_ENTITYID, vHigherClosest3d);
					bool const bLower3dPointInHighestArea = pHigherAreaWithLargestDistance->CalcPointWithin(entityId, vOnLowerHull);
					float fFadeDistanceSq = fLargestDistanceSq;

					// case 1) where approaching the lower area is closer than the higher
					if (!bPosInLowerArea && !bPosInHighestArea)
					{
						float const fDistanceToLower = vOnLowerHull.GetSquaredDistance(rEntityPos);
						float const fDistanceToHigher = vHigherClosest3d.GetSquaredDistance(rEntityPos);

						if (fDistanceToLower >= fDistanceToHigher && (bHighest3dPointInLowerArea || bLower3dPointInHighestArea))
						{
							// best thing would be to take vOnLowerHull and use that Position to calculate a point
							// on HigherHull, check again, if that would be inside the lower, and fade from there.
							fFadeDistanceSq = FLT_MAX; //fDistanceToLower;
						}
						else
						{
							return false;
						}
					}

					float const fDistance = sqrt_tpl(fFadeDistanceSq);
					float fNewFade = (fHigherGreatestFadeDistance > 0.0f) ? max(0.0f, (fHigherGreatestFadeDistance - fDistance) / fHigherGreatestFadeDistance) : 0.0f;
					float const fNewEnvironmentFade = (fHigherGreatestEnvFadeDistance > 0.0f) ? max(0.0f, (fHigherGreatestEnvFadeDistance - fDistance) / fHigherGreatestEnvFadeDistance) : 0.0f;

					// case 4)
					if (bPosInLowerArea && bPosInHighestArea && bHighest3dPointInLowerArea)
					{
						pArea->ExclusiveUpdateAreaInside(pEntity, pHigherAreaWithLargestDistance->GetEntityID(), fNewFade, fNewEnvironmentFade);
					}
					else
					{
						// if point on higher hull is not the same as point on lower hull, then lets not fade in the effect
						if (bPosInHighestArea && vOnLowerHull.GetDistance(vHigherClosest3d) > 0.01f)
						{
							fNewFade = 0.0f;
						}

						pArea->ExclusiveUpdateAreaNear(pEntity, pHigherAreaWithLargestDistance->GetEntityID(), fNewFade);
					}

					bResult = true;
				}
			}
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::NotifyAreas(CArea* const __restrict pArea, SAreasCache const* const pAreaCache, IEntity const* const pEntity)
{
	int const currentGroupId = pArea->GetGroup();

	if (currentGroupId > 0)
	{
		int const currentPriority = pArea->GetPriority();
		int currentHighestPriorityOfLowerPriorityAreas = -1;

		// Notify areas of the same group and next lower priority.
		CArea* pNotifyCandidate = nullptr;
		TAreaPointers areasToNotify;

		for (auto const& areaCacheEntry : pAreaCache->aoAreas)
		{
			pNotifyCandidate = areaCacheEntry.pArea;
			int const notifyAreaPriority = pNotifyCandidate->GetPriority();

#if defined(DEBUG_AREAMANAGER)
			CheckArea(pNotifyCandidate);
#endif // DEBUG_AREAMANAGER

			// Skip the current area or those that are of different group or inactive (hidden) areas.
			if (pNotifyCandidate != pArea &&
			    currentGroupId == pNotifyCandidate->GetGroup() &&
			    currentPriority > notifyAreaPriority &&
			    pNotifyCandidate->IsActive())
			{
				if (notifyAreaPriority > currentHighestPriorityOfLowerPriorityAreas)
				{
					areasToNotify.clear();
					areasToNotify.push_back(pNotifyCandidate);
					currentHighestPriorityOfLowerPriorityAreas = notifyAreaPriority;
				}
				else if (notifyAreaPriority == currentHighestPriorityOfLowerPriorityAreas)
				{
					areasToNotify.push_back(pNotifyCandidate);
				}
			}
		}

		for (auto const pAreaToNotify : areasToNotify)
		{
			// TODO: This can be split into "OnCrossingInto" and "OnCrossingOutOf"
			// as currently "OnCrossingOutOf" is really only of interest.
			pAreaToNotify->OnAreaCrossing(pEntity);
		}
	}

	OnEvent(ENTITY_EVENT_CROSS_AREA, pEntity->GetId(), pArea);
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::ExitArea(IEntity const* const _pEntity, CArea const* const _pArea)
{
	if (_pEntity != nullptr && _pArea != nullptr)
	{
		SAreasCache* const pAreaCache = GetAreaCache(_pEntity->GetId());

		if (pAreaCache != nullptr)
		{
			pAreaCache->vLastUpdatePos = ZERO;

			if (!pAreaCache->aoAreas.empty())
			{
				TAreaCacheVector::iterator Iter(pAreaCache->aoAreas.begin());
				TAreaCacheVector::const_iterator const IterEnd(pAreaCache->aoAreas.end());

				for (; Iter != IterEnd; ++Iter)
				{
					SAreaCacheEntry const& cacheEntry = *Iter;

#if defined(DEBUG_AREAMANAGER)
					CheckArea(cacheEntry.pArea);
#endif    // DEBUG_AREAMANAGER

					if (_pArea == cacheEntry.pArea)
					{
						if (cacheEntry.bInside)
						{
							cacheEntry.pArea->LeaveArea(_pEntity);
							cacheEntry.pArea->LeaveNearArea(_pEntity);
						}
						else if (cacheEntry.bNear)
						{
							cacheEntry.pArea->LeaveNearArea(_pEntity);
						}

						cacheEntry.pArea->OnRemovedFromAreaCache(_pEntity);
						pAreaCache->aoAreas.erase(Iter);

						break;
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::GetEnvFadeValue(SAreasCache const& areaCache, SAreaCacheEntry& areaCacheEntry, Vec3 const& pos, float& outEnvironmentAmount)
{
	bool const bIsPointWithin = (areaCacheEntry.pArea->CalcPosType(INVALID_ENTITYID, pos) == AREA_POS_TYPE_2DINSIDE_ZINSIDE);

	if (bIsPointWithin)
	{
		areaCacheEntry.bInside = true;

		// This updates the distance to closest border when inside the area.
		areaCacheEntry.pArea->CalcPointWithinDist(INVALID_ENTITYID, pos, false);

		if (!GetEnvFadeValueInner(areaCache, areaCacheEntry, pos, outEnvironmentAmount))
		{
			outEnvironmentAmount = 1.0f;
		}
	}
	else
	{
		areaCacheEntry.bInside = false;

		// This is optimized internally and might not recalculate but rather retrieve the cached data.
		Vec3 temp(ZERO);
		float const distanceSq = areaCacheEntry.pArea->CalcPointNearDistSq(INVALID_ENTITYID, pos, temp, false);
		float const greatestFadeDistance = areaCacheEntry.pArea->GetGreatestFadeDistance();
		bool const isNear = ((distanceSq > 0) && (distanceSq < greatestFadeDistance * greatestFadeDistance));

		if (isNear)
		{
			// is near now
			if (!GetEnvFadeValueInner(areaCache, areaCacheEntry, pos, outEnvironmentAmount))
			{
				float const distance = sqrt_tpl(distanceSq);
				float const greatestEnvFadeDistance = areaCacheEntry.pArea->GetGreatestEnvironmentFadeDistance();
				outEnvironmentAmount = (greatestEnvFadeDistance > 0.0f) ? max(0.0f, (greatestEnvFadeDistance - distance) / greatestEnvFadeDistance) : 0.0f;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAreaManager::GetEnvFadeValueInner(SAreasCache const& areaCache, SAreaCacheEntry const& areaCacheEntry, Vec3 const& pos, float& outEnvironmentAmount)
{
	bool bSuccess = false;
	int const currentGroupId = areaCacheEntry.pArea->GetGroup();

	if (currentGroupId > 0)
	{
		// Find areas of higher priority that belong to the same group as the passed pArea.
		int const minPriority = areaCacheEntry.pArea->GetPriority();
		TAreaPointers areasOfSameGroup;

		for (SAreaCacheEntry const& tempAreaCacheEntry : areaCache.aoAreas)
		{
			CArea* const pHigherPrioArea = tempAreaCacheEntry.pArea;

#if defined(DEBUG_AREAMANAGER)
			CheckArea(pHigherPrioArea);
#endif // DEBUG_AREAMANAGER

			// Must be of same group.
			if (currentGroupId == pHigherPrioArea->GetGroup())
			{
				// Only check areas which are active (not hidden).
				if (minPriority < pHigherPrioArea->GetPriority() && pHigherPrioArea->IsActive())
				{
					areasOfSameGroup.push_back(pHigherPrioArea);
				}
			}
		}

		// Continue only if there were areas found of higher priority.
		if (!areasOfSameGroup.empty())
		{
			bool const bPosInLowerArea = areaCacheEntry.bInside;
			float largestDistanceSq = bPosInLowerArea ? 0.0f : FLT_MAX;
			CArea* pHigherAreaWithLargestDistance = nullptr;
			float higherGreatestEnvFadeDistance = 0.0f;
			Vec3 higherClosest3d(ZERO);
			Vec3 tempHigherClosest3d(ZERO);
			bool bPosInHighestArea = false;

			for (CArea* const pHigherArea : areasOfSameGroup)
			{
				// This is optimized internally and might not recalculate but rather retrieve the cached data.
				bool const bPosInHighArea = pHigherArea->CalcPointWithin(INVALID_ENTITYID, pos);
				float const distanceSq = pHigherArea->CalcPointNearDistSq(INVALID_ENTITYID, pos, tempHigherClosest3d, false);

				bool bUseThisArea = distanceSq > 0.0f && ((largestDistanceSq < distanceSq && bPosInLowerArea) || (distanceSq < largestDistanceSq && !bPosInLowerArea));
				// reject cases when Pos is not inside new High Area and we already found a high area
				// and add the case where Pos is inside new High Area, but would be rejected by Higher Area,
				bUseThisArea = (bUseThisArea && !(!bPosInHighArea && bPosInHighestArea)) || (bPosInHighArea && !bPosInHighestArea);

				if (bUseThisArea)
				{
					largestDistanceSq = distanceSq;
					pHigherAreaWithLargestDistance = pHigherArea;
					higherGreatestEnvFadeDistance = pHigherArea->GetGreatestEnvironmentFadeDistance();
					higherClosest3d = tempHigherClosest3d;
					bPosInHighestArea = bPosInHighArea;
				}
			}

			// case 2) if we are in the low area, but not inside the high one, then no need to update exclusive
			if (!bPosInLowerArea || bPosInHighestArea)
			{
				// did we find the best Higher Area to control this Area?
				if (pHigherAreaWithLargestDistance != nullptr)
				{
					bool const bHighest3dPointInLowerArea = areaCacheEntry.pArea->CalcPointWithin(INVALID_ENTITYID, higherClosest3d);
					bool const bLower3dPointInHighestArea = pHigherAreaWithLargestDistance->CalcPointWithin(INVALID_ENTITYID, pos);
					float fadeDistanceSq = largestDistanceSq;

					// case 1) where approaching the lower area is closer than the higher
					if (!bPosInLowerArea && !bPosInHighestArea)
					{
						float const distanceToHigher = higherClosest3d.GetSquaredDistance(pos);

						if (distanceToHigher >= 0.0f && (bHighest3dPointInLowerArea || bLower3dPointInHighestArea))
						{
							// best thing would be to take pos and use that to calculate a point
							// on HigherHull, check again, if that would be inside the lower, and fade from there.
							fadeDistanceSq = FLT_MAX;
						}
					}

					float const distance = sqrt_tpl(fadeDistanceSq);
					outEnvironmentAmount = (higherGreatestEnvFadeDistance > 0.0f) ? max(0.0f, (higherGreatestEnvFadeDistance - distance) / higherGreatestEnvFadeDistance) : 0.0f;
				}

				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

// do onexit for all areas pEntity is in - do it before kill pEntity
//////////////////////////////////////////////////////////////////////////
void CAreaManager::ExitAllAreas(IEntity const* const pEntity)
{
	SAreasCache* const pAreaCache = GetAreaCache(pEntity->GetId());

	if (pAreaCache != nullptr && !pAreaCache->aoAreas.empty())
	{
		for (SAreaCacheEntry const& areaCacheEntry : pAreaCache->aoAreas)
		{
#if defined(DEBUG_AREAMANAGER)
			CheckArea(rCacheEntry.pArea);
#endif // DEBUG_AREAMANAGER

			if (areaCacheEntry.bInside)
			{
				areaCacheEntry.pArea->LeaveArea(pEntity);
				areaCacheEntry.pArea->LeaveNearArea(pEntity);
			}
			else if (areaCacheEntry.bNear)
			{
				areaCacheEntry.pArea->LeaveNearArea(pEntity);
			}

			areaCacheEntry.pArea->OnRemovedFromAreaCache(pEntity);
		}

		pAreaCache->aoAreas.clear();
		DeleteAreaCache(pEntity->GetId());
	}

	m_mapEntitiesToUpdate.erase(pEntity->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::DrawAreas(ISystem const* const pSystem)
{
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
	bool bDraw = CVar::pDrawAreas->GetIVal() != 0;
	size_t const nCountAreasTotal = m_apAreas.size();

	if (bDraw)
	{
		for (size_t aIdx = 0; aIdx < nCountAreasTotal; aIdx++)
			m_apAreas[aIdx]->Draw(aIdx);
	}

	int const nDrawDebugValue = CVar::pDrawAreaDebug->GetIVal();
	bDraw = nDrawDebugValue != 0;

	float const fColor[4] = { 0.0f, 1.0f, 0.0f, 0.7f };

	if (bDraw)
	{
		float fDebugPosY = 10.0f;
		gEnv->pRenderer->Draw2dLabel(10.0f, fDebugPosY, 1.5f, fColor, false, "<AreaManager>");
		fDebugPosY += 20.0f;
		gEnv->pRenderer->Draw2dLabel(30.0f, fDebugPosY, 1.3f, fColor, false, "Entities: %d | Areas in Grid: %d", static_cast<int>(m_mapAreaCache.size()), static_cast<int>(m_areaGrid.GetNumAreas()));
		fDebugPosY += 20.0f;

		TAreaCacheMap::const_iterator const IterAreaCacheEnd(m_mapAreaCache.end());
		TAreaCacheMap::const_iterator IterAreaCache(m_mapAreaCache.begin());

		for (; IterAreaCache != IterAreaCacheEnd; ++IterAreaCache)
		{
			IEntity const* const pEntity = gEnv->pEntitySystem->GetEntity((*IterAreaCache).first);

			if (pEntity != nullptr)
			{
				EntityId const entityId = pEntity->GetId();

				if (nDrawDebugValue == 1 || nDrawDebugValue == 2 || entityId == static_cast<EntityId>(nDrawDebugValue))
				{
					SAreasCache const& areaCache((*IterAreaCache).second);
					Vec3 const& pos(areaCache.vLastUpdatePos);

					gEnv->pRenderer->Draw2dLabel(30.0f, fDebugPosY, 1.3f, fColor, false, "Entity: %d (%s) Pos: (%.2f, %.2f, %.2f) Areas in AreaCache: %d", entityId, pEntity->GetName(), pos.x, pos.y, pos.z, static_cast<int>(areaCache.aoAreas.size()));
					fDebugPosY += 12.0f;

					// Invalidate grid flag in area cache
					for (SAreaCacheEntry const& areaCacheEntry : areaCache.aoAreas)
					{
						CArea* const pArea = areaCacheEntry.pArea;

	#if defined(DEBUG_AREAMANAGER)
						CheckArea(pArea);
	#endif    // DEBUG_AREAMANAGER

						// This is optimized internally and might not recalculate but rather retrieve the cached data.
						float const fDistanceSq = pArea->CalcPointNearDistSq(entityId, pos, false, false);
						bool const bIsPointWithin = (pArea->CalcPosType(entityId, pos) == AREA_POS_TYPE_2DINSIDE_ZINSIDE);

						CryFixedStringT<16> sAreaType("unknown");
						EEntityAreaType const eAreaType = pArea->GetAreaType();

						switch (eAreaType)
						{
						case ENTITY_AREA_TYPE_SHAPE:
							{
								sAreaType = "Shape";

								break;
							}
						case ENTITY_AREA_TYPE_BOX:
							{
								sAreaType = "Box";

								break;
							}
						case ENTITY_AREA_TYPE_SPHERE:
							{
								sAreaType = "Sphere";

								break;
							}
						case ENTITY_AREA_TYPE_GRAVITYVOLUME:
							{
								sAreaType = "GravityVolume";

								break;
							}
						case ENTITY_AREA_TYPE_SOLID:
							{
								sAreaType = "Solid";

								break;
							}
						default:
							{
								sAreaType = "unknown";

								break;
							}
						}

						CryFixedStringT<16> const sState(bIsPointWithin ? "Inside" : (areaCacheEntry.bNear ? "Near" : "Far"));
						gEnv->pRenderer->Draw2dLabel(30.0f, fDebugPosY, 1.3f, fColor, false, "Name: %s AreaID: %d GroupID: %d Priority: %d Type: %s Distance: %.2f State: %s Entities: %d", pArea->GetAreaEntityName(), pArea->GetID(), pArea->GetGroup(), pArea->GetPriority(), sAreaType.c_str(), sqrt_tpl(fDistanceSq > 0.0f ? fDistanceSq : 0.0f), sState.c_str(), static_cast<int>(pArea->GetCacheEntityCount()));
						fDebugPosY += 12.0f;
					}

					// Next entity.
					fDebugPosY += 12.0f;
				}
			}
		}
	}
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::DrawGrid()
{
	m_areaGrid.Draw();
}

//////////////////////////////////////////////////////////////////////////
unsigned CAreaManager::MemStat()
{
	unsigned memSize = sizeof *this;

	unsigned int const nCount = m_apAreas.size();
	for (unsigned int aIdx = 0; aIdx < nCount; aIdx++)
		memSize += m_apAreas[aIdx]->MemStat();

	return memSize;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::ResetAreas()
{
	for (TAreaCacheMap::iterator cacheIt = m_mapAreaCache.begin(), cacheItEnd = m_mapAreaCache.end();
	     cacheIt != cacheItEnd;
	     ++cacheIt)
	{
		EntityId entityId = cacheIt->first;
		SAreasCache const& areaCache((*cacheIt).second);
		IEntity* const pEntity = m_pEntitySystem->GetEntity(entityId);

		if (pEntity != nullptr)
		{
			for (SAreaCacheEntry const& areaCacheEntry : areaCache.aoAreas)
			{
				CArea* const pArea = areaCacheEntry.pArea;

#if defined(DEBUG_AREAMANAGER)
				CheckArea(pArea);
#endif  // DEBUG_AREAMANAGER

				if (areaCacheEntry.bInside)
				{
					pArea->LeaveArea(pEntity);
					pArea->LeaveNearArea(pEntity);
				}
				else if (areaCacheEntry.bNear)
				{
					pArea->LeaveNearArea(pEntity);
				}
			}
		}
	}

	m_mapAreaCache.clear();
	stl::free_container(m_mapEntitiesToUpdate);

	// invalidate cached event + data
	size_t const nCount = m_apAreas.size();

	for (size_t i = 0; i < nCount; i++)
	{
		m_apAreas[i]->ClearCachedEvents();
		m_apAreas[i]->ReleaseCachedAreaData();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::UnloadAreas()
{
	ResetAreas();
	m_areaGrid.Reset();
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::SetAreasDirty()
{
	m_bAreasDirty = true;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::SetAreaDirty(IArea* pArea)
{
	if (!m_bAreasDirty)
	{
		bool const success = m_areaGrid.ResetArea(static_cast<CArea*>(pArea));

		if (!success)
			m_bAreasDirty = true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::UpdateDirtyAreas()
{
	if (m_bAreasDirty)
	{
		m_areaGrid.Compile(m_pEntitySystem, m_apAreas);
		m_bAreasDirty = false;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::AddEventListener(IAreaManagerEventListener* pListener)
{
	stl::push_back_unique(m_EventListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::RemoveEventListener(IAreaManagerEventListener* pListener)
{
	stl::find_and_erase(m_EventListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::OnEvent(EEntityEvent event, EntityId TriggerEntityID, IArea* pArea)
{
	if (!m_EventListeners.empty())
	{
		TAreaManagerEventListenerVector::iterator ItEnd = m_EventListeners.end();
		for (TAreaManagerEventListenerVector::iterator It = m_EventListeners.begin(); It != ItEnd; ++It)
		{
			assert(*It);
			(*It)->OnAreaManagerEvent(event, TriggerEntityID, pArea);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
int CAreaManager::GetNumberOfPlayersNearOrInArea(CArea const* const pArea)
{
	// Find the area index
	TAreaPointers::const_iterator IterAreas(m_apAreas.begin());
	TAreaPointers::const_iterator const IterAreasEnd(m_apAreas.end());

	for (; IterAreas != IterAreasEnd; ++IterAreas)
	{
		if (pArea == (*IterAreas))
		{
			// Now find how many players are actually inside the area
			int nPlayersInAreaCount = 0;
			TAreaCacheMap::iterator Iter(m_mapAreaCache.begin());
			TAreaCacheMap::const_iterator IterEnd(m_mapAreaCache.end());

			for (; Iter != IterEnd; ++Iter)
			{
				SAreasCache& rAreaCache((*Iter).second);
				SAreaCacheEntry* pAreaCacheEntry = nullptr;

				if (rAreaCache.GetCacheEntry(pArea, &pAreaCacheEntry) && (pAreaCacheEntry->bInside || pAreaCacheEntry->bNear))
				{
					++nPlayersInAreaCount;
				}
			}

			return nPlayersInAreaCount;
		}
	}

	// If we get here then we've failed to find the area - should never happen
	return 0;
}

//////////////////////////////////////////////////////////////////////////
size_t CAreaManager::GetOverlappingAreas(const AABB& bb, PodArray<IArea*>& list) const
{
	const CArea::TAreaBoxes& boxes = CArea::GetBoxHolders();
	for (size_t i = 0, end = boxes.size(); i < end; ++i)
	{
		const CArea::SBoxHolder& holder = boxes[i];
		if (!holder.box.Overlaps2D(bb))
			continue;
		list.push_back(holder.area);
	}

	return list.Size();
}

//////////////////////////////////////////////////////////////////////////
bool CAreaManager::SIsNotInGrid::operator()(SAreaCacheEntry const& rCacheEntry) const
{
	bool bResult = false;

#if defined(DEBUG_AREAMANAGER)
	if (!stl::find(rapAreas, rCacheEntry.pArea))
	{
		CryFatalError("<AreaManager>: area not found in overall areas list!");
	}
#endif // DEBUG_AREAMANAGER

	if (!rCacheEntry.bInGrid)
	{
		rCacheEntry.pArea->OnRemovedFromAreaCache(pEntity);
		bResult = true;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CAreaManager::SRemoveIfNoAreasLeft::operator()(VectorMap<EntityId, SAreasCache>::value_type& rCacheEntry) const
{
	bool bResult = false;
	TAreaCacheVector::iterator Iter(rCacheEntry.second.aoAreas.begin());
	TAreaCacheVector::const_iterator const IterEnd(rCacheEntry.second.aoAreas.end());

	for (; Iter != IterEnd; ++Iter)
	{
		SAreaCacheEntry const& rAreaCacheEntry = (*Iter);

#if defined(DEBUG_AREAMANAGER)
		if (!stl::find(rapAreas, rAreaCacheEntry.pArea))
		{
			CryFatalError("<AreaManager>: area not found in overall areas list!");
		}
#endif // DEBUG_AREAMANAGER

		if (rAreaCacheEntry.pArea == pArea)
		{
			rCacheEntry.second.aoAreas.erase(Iter);
			bResult = rCacheEntry.second.aoAreas.empty();

			break;
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
	case ESYSTEM_EVENT_SW_FORCE_LOAD_END:
	case ESYSTEM_EVENT_SW_SHIFT_WORLD:
		// This seems to be enabled by Segmented World, which is turned of in ProjectDefines.h
		// if(GetEntitySystem()->EntitiesUseGUIDs())
		{
			const int numAreasTotal = m_apAreas.size();
			for (int nAreaIndex = 0; nAreaIndex < numAreasTotal; ++nAreaIndex)
			{
				m_apAreas[nAreaIndex]->ResolveEntityIds();
			}
		}
		break;
	}
}
