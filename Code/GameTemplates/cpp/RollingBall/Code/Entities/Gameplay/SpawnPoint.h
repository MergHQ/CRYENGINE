#pragma once

#include <CryEntitySystem/IEntityComponent.h>

////////////////////////////////////////////////////////
// Entity responsible for spawning other entities
////////////////////////////////////////////////////////
class CSpawnPoint : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CSpawnPoint,
		"CSpawnPoint", 0xB3EFD9F28FA840F9, 0x8F7B4BDF75C3A119);
public:
	virtual ~CSpawnPoint() {}

	void SpawnEntity(IEntity &otherEntity);
};
