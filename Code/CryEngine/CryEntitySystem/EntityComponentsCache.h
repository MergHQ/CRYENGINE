// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#ifndef RELEASE
#include <CrySchematyc/Utils/ClassProperties.h>

//! Entity components cache stores the states of components as they are edited during game mode in Sandbox
//! This allows us to restore state when switching back to editing mode
class CEntityComponentsCache
{
public:
	void StoreEntities();
	void RestoreEntities();

	void OnEntitySpawnedDuringGameMode(const EntityId id) { m_entitiesSpawnedDuringEditorGameMode.emplace(id); }
	void OnEntityRemovedDuringGameMode(const EntityId id) {	m_entitiesSpawnedDuringEditorGameMode.erase(id); }
	void RemoveEntitiesSpawnedDuringGameMode();

private:
	void StoreEntity(CEntity& entity);
	void LoadEntity(CEntity& entity);
	void StoreComponent(CEntity& entity, IEntityComponent& component);
	void LoadComponent(CEntity& entity, IEntityComponent& component);

private:
	struct SDoubleGUID
	{
		CryGUID guid1;
		CryGUID guid2;
		bool operator==(const SDoubleGUID& rhs) const { return guid1 == rhs.guid1 && guid2 == rhs.guid2; }
	};
	struct HashDoubleGUID
	{
		size_t operator()(const SDoubleGUID& doubleGuid) const
		{
			std::hash<uint64> hasher;
			return hasher(doubleGuid.guid1.lopart) ^ hasher(doubleGuid.guid1.hipart) ^ hasher(doubleGuid.guid2.lopart) ^ hasher(doubleGuid.guid2.hipart);
		}
	};

	std::unordered_map<SDoubleGUID, std::unique_ptr<Schematyc::CClassProperties>, HashDoubleGUID> m_componentPropertyCache;
	std::unordered_set<EntityId> m_entitiesSpawnedDuringEditorGameMode;
};
#endif