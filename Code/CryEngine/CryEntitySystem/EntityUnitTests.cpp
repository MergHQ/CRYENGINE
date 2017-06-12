// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Entity.h"
#include "EntitySystem.h"

#include <CrySystem/CryUnitTest.h>

#include "SubstitutionProxy.h"

CRY_UNIT_TEST_SUITE(EntityTestsSuit)
{
	CRY_UNIT_TEST(SpawnTest)
	{
		SEntitySpawnParams params;
		params.guid = CryGUID::Create();
		params.sName = "TestEntity";
		params.nFlags = ENTITY_FLAG_CLIENT_ONLY;
		params.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

		IEntity *pEntity = gEnv->pEntitySystem->SpawnEntity(params);
		EntityId id = pEntity->GetId();
		CRY_UNIT_TEST_ASSERT(pEntity != NULL);

		CRY_UNIT_TEST_ASSERT( pEntity->GetGuid() != CryGUID::Null() );

		CRY_UNIT_TEST_CHECK_EQUAL(id, gEnv->pEntitySystem->FindEntityByGuid(pEntity->GetGuid()));
		CRY_UNIT_TEST_CHECK_EQUAL(pEntity, gEnv->pEntitySystem->FindEntityByName(params.sName));

		// Test Entity components
		IEntitySubstitutionComponent *pComponent = pEntity->GetOrCreateComponent<IEntitySubstitutionComponent>();
		CRY_UNIT_TEST_ASSERT( nullptr != pComponent );
		CRY_UNIT_TEST_ASSERT( 1 == pEntity->GetComponentsCount() );
	}
}
