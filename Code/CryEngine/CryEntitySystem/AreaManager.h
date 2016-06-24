// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Containers/VectorMap.h>
#include "AreaGrid.h"

//#define DEBUG_AREAMANAGER

class CEntitySystem;
class CArea;
struct IVisArea;
struct IAreaManagerEventListener;

typedef std::vector<IAreaManagerEventListener*> AreaManagerEventListenerVector;
typedef std::vector<SAudioAreaInfo>             AreaEnvironments;

//Areas manager
class CAreaManager : public IAreaManager, public ISystemEventListener
{
	struct SAreaCacheEntry
	{
		SAreaCacheEntry()
			: pArea(NULL),
			bNear(false),
			bInside(false),
			bInGrid(true)
		{};

		SAreaCacheEntry(CArea* const pPassedArea, bool const bIsNear, bool const bIsInside)
			: pArea(pPassedArea),
			bNear(bIsNear),
			bInside(bIsInside),
			bInGrid(true)
		{};

		SAreaCacheEntry& operator=(SAreaCacheEntry const& rOther)
		{
			pArea = rOther.pArea;
			bInGrid = rOther.bInGrid;
			bInside = rOther.bInside;
			bNear = rOther.bNear;
			return *this;
		}

		CArea* pArea;
		bool   bInGrid;
		bool   bInside;
		bool   bNear;
	};

	typedef std::vector<SAreaCacheEntry> TAreaCacheVector;

	struct SAreasCache
	{
		SAreasCache()
			: lastUpdatePos(ZERO)
		{}

		bool GetCacheEntry(CArea const* const pAreaToFind, SAreaCacheEntry** const ppAreaCacheEntry)
		{
			bool bSuccess = false;
			* ppAreaCacheEntry = nullptr;

			for (auto& entry : entries)
			{
				if (entry.pArea == pAreaToFind)
				{
					* ppAreaCacheEntry = &entry;
					bSuccess = true;

					break;
				}
			}

			return bSuccess;
		}

		TAreaCacheVector entries;
		Vec3             lastUpdatePos;
	};

	typedef VectorMap<EntityId, SAreasCache> TAreaCacheMap;
	typedef VectorMap<EntityId, size_t>      TEntitiesToUpdateMap;

public:

	explicit CAreaManager(CEntitySystem* pEntitySystem);
	~CAreaManager(void);

	//IAreaManager
	virtual size_t             GetAreaAmount() const override { return m_areas.size(); }
	virtual IArea const* const GetArea(size_t const nAreaIndex) const override;
	virtual size_t             GetOverlappingAreas(const AABB& bb, PodArray<IArea*>& list) const override;
	virtual void               SetAreasDirty() override;
	virtual void               SetAreaDirty(IArea* pArea) override;
	virtual void               ExitAllAreas(IEntity const* const pEntity) override;
	//~IAreaManager

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	// Makes a new area.
	CArea*         CreateArea();

	CEntitySystem* GetEntitySystem() const { return m_pEntitySystem; };

	// Puts the passed entity ID into the update list for the next update.
	virtual void MarkEntityForUpdate(EntityId const nEntityID) override;

	virtual void TriggerAudioListenerUpdate(IArea const* const _pArea) override;

	// Called every frame.
	void Update();

	bool ProceedExclusiveUpdateByHigherArea(
	  SAreasCache* const pAreaCache,
	  IEntity const* const pIEntity,
	  Vec3 const& entityPos,
	  CArea* const pArea,
	  Vec3 const& onLowerHull,
	  AreaEnvironments& areaEnvironments);

	void         NotifyAreas(CArea* const __restrict pArea, SAreasCache const* const pAreaCache, IEntity const* const pEntity);

	virtual void DrawLinkedAreas(EntityId linkedId) const override;
	size_t       GetLinkedAreas(EntityId linkedId, int areaId, std::vector<CArea*>& areas) const;

	virtual bool GetLinkedAreas(EntityId linkedId, EntityId* pOutArray, int& outAndMaxResults) const override;

	void         DrawAreas(const ISystem* const pSystem);
	void         DrawGrid();
	size_t       MemStat();
	void         ResetAreas();
	void         UnloadAreas();

	virtual bool QueryAudioAreas(Vec3 const& pos, SAudioAreaInfo* const pResults, size_t const numMaxResults, size_t& outNumResults) override;

	// Register EventListener to the AreaManager.
	virtual void AddEventListener(IAreaManagerEventListener* pListener) override;
	virtual void RemoveEventListener(IAreaManagerEventListener* pListener) override;

	//! Fires event for all listeners to this sound.
	void OnEvent(EEntityEvent event, EntityId TriggerEntityID, IArea* pArea);

	int  GetNumberOfPlayersNearOrInArea(CArea const* const pArea);

protected:

	friend class CArea;
	void Unregister(CArea const* const pArea);

	// Holds all registered areas.
	TAreaPointers  m_areas;

	CEntitySystem* m_pEntitySystem;
	bool           m_bAreasDirty;
	CAreaGrid      m_areaGrid;

private:

	AreaManagerEventListenerVector m_EventListeners;

	//////////////////////////////////////////////////////////////////////////
	SAreasCache* GetAreaCache(EntityId const nEntityId)
	{
		SAreasCache* pAreaCache = nullptr;
		TAreaCacheMap::iterator const Iter(m_mapAreaCache.find(nEntityId));

		if (Iter != m_mapAreaCache.end())
		{
			pAreaCache = &(Iter->second);
		}

		return pAreaCache;
	}

	//////////////////////////////////////////////////////////////////////////
	SAreasCache* MakeAreaCache(EntityId const nEntityID)
	{
		SAreasCache* pAreaCache = &m_mapAreaCache[nEntityID];
		return pAreaCache;
	}

	//////////////////////////////////////////////////////////////////////////
	void DeleteAreaCache(EntityId const nEntityId)
	{
		m_mapAreaCache.erase(nEntityId);
	}

	void UpdateEntity(Vec3 const& position, IEntity* const pIEntity);
	void UpdateDirtyAreas();
	void ProcessArea(CArea* const pArea, SAreaCacheEntry& areaCacheEntry, SAreasCache* const pAreaCache, Vec3 const& pos, IEntity const* const pIEntity, AreaEnvironments& areaEnvironments);
	void ExitArea(IEntity const* const _pEntity, CArea const* const _pArea);
	bool GetEnvFadeValue(SAreasCache const& areaCache, SAreaCacheEntry& areaCacheEntry, Vec3 const& entityPos, EntityId const envProvidingEntityId, AreaEnvironments& areaEnvironments);
	bool GetEnvFadeValueInner(SAreasCache const& areaCache, SAreaCacheEntry const& areaCacheEntry, Vec3 const& entityPos, Vec3 const& posOnLowerArea, EntityId const envProvidingEntityId, AreaEnvironments& areaEnvironments);
	bool RetrieveEnvironmentAmount(CArea const* const pArea, float const amount, float const distance, EntityId const envProvidingEntityId, AreaEnvironments& areaEnvironments);

	// Unary predicates for conditional removing!
	static inline bool IsDoneUpdating(std::pair<EntityId, size_t> const& rEntry)
	{
		return rEntry.second == 0;
	}

	struct SIsNotInGrid
	{
		SIsNotInGrid(IEntity const* const pPassedEntity, std::vector<CArea*> const& rapPassedAreas, size_t const nPassedCountAreas)
			: pEntity(pPassedEntity),
			rapAreas(rapPassedAreas),
			nCountAreas(nPassedCountAreas){}

		bool operator()(SAreaCacheEntry const& rCacheEntry) const;

		IEntity const* const       pEntity;
		std::vector<CArea*> const& rapAreas;
		size_t const               nCountAreas;
	};

	struct SRemoveIfNoAreasLeft
	{
		SRemoveIfNoAreasLeft(CArea const* const pPassedArea, std::vector<CArea*> const& rapPassedAreas, size_t const nPassedCountAreas)
			: pArea(pPassedArea),
			rapAreas(rapPassedAreas),
			nCountAreas(nPassedCountAreas){}

		bool operator()(VectorMap<EntityId, SAreasCache>::value_type& rCacheEntry) const;

		CArea const* const         pArea;
		std::vector<CArea*> const& rapAreas;
		size_t const               nCountAreas;
	};

	TAreaCacheMap        m_mapAreaCache;          // Area cache per entity id.
	TEntitiesToUpdateMap m_mapEntitiesToUpdate;

#if defined(DEBUG_AREAMANAGER)
	//////////////////////////////////////////////////////////////////////////
	void CheckArea(CArea const* const pArea)
	{
		if (!stl::find(m_areas, pArea))
		{
			CryFatalError("<AreaManager>: area not found in overall areas list!");
		}
	}
#endif // DEBUG_AREAMANAGER
};
