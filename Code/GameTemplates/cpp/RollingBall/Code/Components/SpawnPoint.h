#pragma once
#include <CryEntitySystem/IEntityComponent.h>
#include <CryEntitySystem/IEntity.h>

////////////////////////////////////////////////////////
// Spawn point to spawn the player
////////////////////////////////////////////////////////
class CSpawnPointComponent final : public IEntityComponent
{
public:
	CSpawnPointComponent() = default;
	virtual ~CSpawnPointComponent() {}

	static void ReflectType(Schematyc::CTypeDesc<CSpawnPointComponent>& desc);

	static CryGUID& IID()
	{
		static CryGUID id = "{41316132-8A1E-4073-B0CD-A242FD3D2E90}"_cry_guid;
		return id;
	}

	static Vec3 GetFirstSpawnPointPos()
	{
		auto *pEntityIterator = gEnv->pEntitySystem->GetEntityIterator();
		pEntityIterator->MoveFirst();

		while (!pEntityIterator->IsEnd())
		{
			IEntity *pEntity = pEntityIterator->Next();

			if (IEntityComponent* pSpawner = pEntity->GetComponent<CSpawnPointComponent>())
			{
				return pSpawner->GetEntity()->GetPos();
			}
		}
		return Vec3(ZERO);
	}
};
