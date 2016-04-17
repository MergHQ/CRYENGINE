// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: A pool of entity containers with matching signatures

   -------------------------------------------------------------------------
   History:
   - 12:04:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __ENTITYPOOL_H__
#define __ENTITYPOOL_H__

#include "EntityPoolCommon.h"
#include "EntityPoolSignature.h"

class CEntityPool
{
public:
	CEntityPool();
	CEntityPool(CEntityPoolManager* pEntityPoolManager, TEntityPoolId id);
	~CEntityPool();
	void                        GetMemoryStatistics(ICrySizer* pSizer) const;

	TEntityPoolId               GetId() const           { return m_Id; }
	bool                        HasAI() const           { return m_bDefinitionHasAI; }
	uint32                      GetSize() const         { return m_InactivePoolIds.size() + m_ActivePoolIds.size(); }
	const string&               GetName() const         { return m_sName; }
	const string&               GetDefaultClass() const { return m_sDefaultClass; }
	const CEntityPoolSignature& GetSignature() const    { return m_Signature; }

	void                        CreateSpawnParams(SEntitySpawnParams& params, EntityId entityId = 0, bool bUsePoolId = false) const;

	bool                        CreatePool(const CEntityPoolDefinition& definition);
	void                        ResetPool(bool bSaveState);
	void                        DestroyPool();
	bool                        ConsumePool(CEntityPool& otherPool);

	bool                        ContainsEntityClass(IEntityClass* pClass) const;
	CEntity*                    GetEntityFromPool(bool& outIsActive, EntityId forcedPoolId = 0);

	bool                        ContainsEntity(EntityId entityId) const;
	bool                        IsInInactiveList(EntityId entityId) const;
	bool                        IsInActiveList(EntityId entityId) const;
	EntityId                    GetPoolId(EntityId usingId) const;

	void                        OnPoolEntityInUse(CEntity* pEntity, EntityId prevId);
	void                        OnPoolEntityReturned(CEntity* pEntity, EntityId prevId);

	// Serialization helpers
	void UpdateActiveEntityBookmarks();

#ifdef ENTITY_POOL_DEBUGGING
	void DebugDraw(IRenderer* pRenderer, float& fColumnX, float& fColumnY, bool bExtraInfo) const;
#endif //ENTITY_POOL_DEBUGGING

private:
	// Pool management
	bool CreatePoolEntity(CEntity*& pOutEntity, bool bAddToInactiveList = true);
	bool RemovePoolEntity(EntityId entityId);
	bool DestroyPoolEntity(EntityId entityId) const;

	void ReturnAllActive(bool bSaveState);

	// Pool accessing
	CEntity* GetPoolEntityWithPoolId(bool& outIsActive, EntityId forcedPoolId);
	CEntity* GetPoolEntityFromInactiveSet();
	CEntity* GetPoolEntityFromActiveSet();

private:
	TEntityPoolId        m_Id;

	bool                 m_bDefinitionHasAI;
	uint32               m_uMaxSize;
	CEntityPoolManager*  m_pEntityPoolManager;
	CEntityPoolSignature m_Signature;
	string               m_sName;
	string               m_sDefaultClass;

	typedef std::vector<TEntityPoolDefinitionId> TPoolDefinitionIds;
	TPoolDefinitionIds m_PoolDefinitionIds;

	struct SEntityIds
	{
		EntityId poolId;
		EntityId usingId;

		SEntityIds() : poolId(0), usingId(0) {}
		SEntityIds(EntityId _poolId) : poolId(_poolId), usingId(_poolId) {}
		SEntityIds(EntityId _poolId, EntityId _usingId) : poolId(_poolId), usingId(_usingId) {}

		const bool operator==(const SEntityIds& other) const { return usingId == other.usingId; }
		const bool operator!=(const SEntityIds& other) const { return !(*this == other); }
		const bool operator<(const SEntityIds& other) const  { return poolId < other.poolId; }

		/*const bool operator ()(const SEntityIds& a, EntityId otherId) const
		   {
		   return a.usingId == otherId;
		   }*/
		class CompareUsingIds : public std::binary_function<SEntityIds, EntityId, bool>
		{
		public:
			bool operator()(const SEntityIds& self, const EntityId id) const
			{
				return self.usingId == id;
			}
		};
	};
	typedef std::vector<SEntityIds> TPoolIdsVec;
	TPoolIdsVec m_InactivePoolIds;
	TPoolIdsVec m_ActivePoolIds;

	struct SReturnToPoolWeight
	{
		CEntity* pEntity;
		float    weight;

		SReturnToPoolWeight() : pEntity(NULL), weight(0.0f) {}
		const bool operator<(const SReturnToPoolWeight& other) const { return !(weight <= other.weight); }
	};
	typedef std::vector<SReturnToPoolWeight> TReturnToPoolWeights;
	TReturnToPoolWeights m_ReturnToPoolWeights;

#ifdef ENTITY_POOL_DEBUGGING
	int  m_iDebug_TakenFromActiveForced;
	bool m_bDebug_HasExpanded;
#endif //ENTITY_POOL_DEBUGGING
};

#endif //__ENTITYPOOL_H__
