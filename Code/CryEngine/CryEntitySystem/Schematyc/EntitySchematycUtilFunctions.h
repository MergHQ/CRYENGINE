// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "Entity.h"
#include "EntitySystem.h"
#include <CrySerialization/Forward.h>
#include <CryMath/Cry_Camera.h>

#include  <CrySchematyc/Env/Elements/EnvFunction.h>
#include  <CrySchematyc/Utils/SharedString.h>
#include  <CrySchematyc/IObject.h>
#include  <CrySchematyc/Env/IEnvRegistrar.h>

namespace Schematyc
{
namespace Entity
{
CSharedString GetEntityName(ExplicitEntityId entityId)
{
	const CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId));
	return pEntity ? CSharedString(pEntity->GetName()) : CSharedString();
}

ObjectId GetEntityObjectId(ExplicitEntityId entityId)
{
	const CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId));
	return (pEntity && pEntity->GetSchematycObject()) ? pEntity->GetSchematycObject()->GetId() : ObjectId::Invalid;
}

ExplicitEntityId FindEntityByName(CSharedString name)
{
	const CEntity* pEntity = static_cast<CEntity*>(g_pIEntitySystem->FindEntityByName(name.c_str()));
	return ExplicitEntityId(pEntity ? pEntity->GetId() : INVALID_ENTITYID);
}

void Hide(ExplicitEntityId entityId, bool bHide)
{
	if (CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		pEntity->Hide(bHide);
	}
}

bool IsHidden(ExplicitEntityId entityId)
{
	if (const CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		return pEntity->IsHidden();
	}

	return true;
}

void SetTransform(ExplicitEntityId entityId, const CryTransform::CTransform& transform)
{
	if (CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		pEntity->SetWorldTM(transform.ToMatrix34());
	}
}

CryTransform::CTransform GetTransform(ExplicitEntityId entityId)
{
	if (const CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		return CryTransform::CTransform(pEntity->GetWorldTM());
	}

	return CryTransform::CTransform();
}

void SetRotation(ExplicitEntityId entityId, const CryTransform::CRotation& rotation)
{
	if (CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		Matrix34 transform = pEntity->GetWorldTM();
		transform.SetRotation33(rotation.ToMatrix33());
		pEntity->SetWorldTM(transform);
	}
}

CryTransform::CRotation GetRotation(ExplicitEntityId entityId)
{
	if (const CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		return CryTransform::CRotation(pEntity->GetWorldRotation());
	}

	return CryTransform::CRotation();
}

void SetPosition(ExplicitEntityId entityId, const Vec3& position)
{
	if (CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		Matrix34 transform = pEntity->GetWorldTM();
		transform.SetTranslation(position);
		pEntity->SetWorldTM(transform);
	}
}

Vec3 GetPosition(ExplicitEntityId entityId)
{
	if (const CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(static_cast<EntityId>(entityId)))
	{
		return pEntity->GetWorldPos();
	}

	return ZERO;
}

static void RegisterUtilFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<ExplicitEntityId>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&GetEntityName, "955ca6c4-5b0a-4150-aaba-79e2939e85f7"_cry_guid, "GetEntityName");
		pFunction->SetDescription("Get name of entity");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindOutput(0, 'name', "Name");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&GetEntityObjectId, "cdd011ac-14a1-403b-af21-28d9e77e562d"_cry_guid, "GetEntityObjectId");
		pFunction->SetDescription("Get object id from entity");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindOutput(0, 'obj', "ObjectId");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&FindEntityByName, "{823518D1-6FE9-49DA-A522-C1483385B70D}"_cry_guid, "FindEntityByName");
		pFunction->SetDescription("Finds an entity by name");
		pFunction->BindInput(1, 'name', "Name");
		pFunction->BindOutput(0, 'ent', "EntityId");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Hide, "abc4938d-a631-4a36-9f10-22cf6dc9dabd"_cry_guid, "Hide");
		pFunction->SetDescription("Show/hide entity");
		pFunction->SetFlags(EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindInput(2, 'vis', "Hide");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&IsHidden, "5aa5e8f0-b4f4-491d-8074-d8b129500d09"_cry_guid, "IsHidden");
		pFunction->SetDescription("Is entity hidden?");
		pFunction->SetFlags(EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindOutput(0, 'vis', "Visible");
		scope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&SetTransform, "FA08CEA0-A0C5-4340-9F8A-E38D74488BAF"_cry_guid, "SetTransform");
		pFunction->SetDescription("Set Entity Transformation");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindInput(2, 'tr', "transform");
		scope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&GetTransform, "8A99E1BA-A5CD-4DE8-A19F-D07DF5D3B245"_cry_guid, "GetTransform");
		pFunction->SetDescription("Get Entity Transformation");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindOutput(0, 'tr', "transform");
		scope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&SetRotation, "{53FDFFFB-A216-4001-BA26-9E81A7D2160D}"_cry_guid, "SetRotation");
		pFunction->SetDescription("Set Entity Rotation");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindInput(2, 'rot', "rotation");
		scope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&GetRotation, "{B03F7198-583E-4C9C-BDC7-92D904920D2C}"_cry_guid, "GetRotation");
		pFunction->SetDescription("Get Entity Rotation");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindOutput(0, 'rot', "rotation");
		scope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&SetPosition, "{1FD3B799-D029-480A-8D7B-F4EDFFF3D5F9}"_cry_guid, "SetPosition");
		pFunction->SetDescription("Set Entity Position");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindInput(2, 'pos', "Position");
		scope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&GetPosition, "{29C87754-67BF-401A-9E8E-F9DCC178031E}"_cry_guid, "GetPosition");
		pFunction->SetDescription("Get Entity Position");
		pFunction->BindInput(1, 'ent', "EntityId");
		pFunction->BindOutput(0, 'pos', "Position");
		scope.Register(pFunction);
	}
}
} // Entity
} // Schematyc