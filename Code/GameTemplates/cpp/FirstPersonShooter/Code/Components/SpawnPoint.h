#pragma once
#include <CryEntitySystem/IEntityComponent.h>
#include <CryEntitySystem/IEntity.h>

////////////////////////////////////////////////////////
// Spawn point
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

public:
	void SpawnEntity(IEntity* otherEntity);
};
