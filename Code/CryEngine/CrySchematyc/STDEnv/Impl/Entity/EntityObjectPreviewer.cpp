// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObjectPreviewer.h"

#include <Cry3DEngine/I3DEngine.h>
#include <CryEntitySystem/IEntityRenderState.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <Schematyc/Entity/EntityUserData.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "STDEnv.h"
#include "Entity/EntityObjectMap.h"

namespace Schematyc
{
void CEntityObjectPreviewer::SerializeProperties(Serialization::IArchive& archive) {}

ObjectId CEntityObjectPreviewer::CreateObject(const SGUID& classGUID) const
{
	IRuntimeClassConstPtr pRuntimeClass = gEnv->pSchematyc->GetRuntimeRegistry().GetClass(classGUID);
	if (pRuntimeClass)
	{
		IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pRuntimeClass->GetName()); // #SchematycTODO : Query directly from CEntityObjectClassRegistry?
		if (pEntityClass)
		{
			static const SEntityUserData userData(true);

			SEntitySpawnParams entitySpawnParams;
			entitySpawnParams.pClass = pEntityClass;
			entitySpawnParams.sName = "Preview";
			entitySpawnParams.pUserData = const_cast<SEntityUserData*>(&userData);

			IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(entitySpawnParams, true);
			if (pEntity)
			{
				return CSTDEnv::GetInstance().GetEntityObjectMap().GetEntityObjectId(pEntity->GetId());
			}
		}
	}
	return ObjectId();
}

ObjectId CEntityObjectPreviewer::ResetObject(ObjectId objectId) const
{
	// #SchematycTODO : Instead of re-creating the preview entity we should send a reset event, but that doesn't seem too update the entity's transform heirarchy / visual state reliably.
	IObject* pObject = gEnv->pSchematyc->GetObject(objectId);
	if (pObject)
	{
		const SGUID classGUID = pObject->GetClass().GetGUID();
		DestroyObject(objectId);
		return CreateObject(classGUID);
	}
	/*
	IObject* pObject = gEnv->pSchematyc->GetObject(objectId);
	if (pObject)
	{
		IEntity& entity = EntityUtils::GetEntity(*pObject);

		SEntityEvent entityEvent(ENTITY_EVENT_RESET);
		entity.SendEvent(entityEvent);

		IEntityRenderProxy* pEntityRenderProxy = static_cast<IEntityRenderProxy*>(entity.GetProxy(ENTITY_PROXY_RENDER));
		if (pEntityRenderProxy)
		{
			pEntityRenderProxy->GetRenderNode()->InvalidatePermanentRenderObject();
		}
	}
	*/
	return objectId;
}

void CEntityObjectPreviewer::DestroyObject(ObjectId objectId) const
{
	IObject* pObject = gEnv->pSchematyc->GetObject(objectId);
	if (pObject)
	{
		IEntity& entity = EntityUtils::GetEntity(*pObject);
		gEnv->pEntitySystem->RemoveEntity(entity.GetId());
	}
}

Sphere CEntityObjectPreviewer::GetObjectBounds(ObjectId objectId) const
{
	IObject* pObject = gEnv->pSchematyc->GetObject(objectId);
	if (pObject)
	{
		IEntity& entity = EntityUtils::GetEntity(*pObject);
		AABB worldBounds;
		entity.GetWorldBounds(worldBounds);
		return Sphere(worldBounds.GetCenter(), worldBounds.GetRadius());
	}
	return Sphere(Vec3(ZERO), 0.0f);
}

void CEntityObjectPreviewer::RenderObject(const IObject& object, const SRendParams& params, const SRenderingPassInfo& passInfo) const
{
	IEntity& entity = EntityUtils::GetEntity(object);
	IEntityRenderProxy* pEntityRenderProxy = static_cast<IEntityRenderProxy*>(entity.GetProxy(ENTITY_PROXY_RENDER));
	if (pEntityRenderProxy)
	{
		pEntityRenderProxy->GetRenderNode()->Render(params, passInfo);
	}
	gEnv->pRenderer->GetIRenderAuxGeom()->Flush();
}
} // Schematyc
