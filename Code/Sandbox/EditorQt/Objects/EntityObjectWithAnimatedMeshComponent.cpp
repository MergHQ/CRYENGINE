// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObjectWithAnimatedMeshComponent.h"

#include <DefaultComponents/Geometry/AnimatedMeshComponent.h>
#include <DefaultComponents/Physics/RigidBodyComponent.h>

using Cry::DefaultComponents::CAnimatedMeshComponent;
using Cry::DefaultComponents::CRigidBodyComponent;

REGISTER_CLASS_DESC(CEntityObjectWithAnimatedMeshComponentDesc);
IMPLEMENT_DYNCREATE(CEntityObjectWithAnimatedMeshComponent, CEntityObject)

bool CEntityObjectWithAnimatedMeshComponent::Init(CBaseObject* prev, const string& file)
{
	if (prev != nullptr)
	{
		if (IEntity* pEntity = static_cast<CEntityObject*>(prev)->GetIEntity())
		{
			if (CAnimatedMeshComponent* pMeshComponent = static_cast<CEntityObject*>(prev)->GetIEntity()->GetComponent<CAnimatedMeshComponent>())
			{
				m_file = pMeshComponent->GetCharacterFile();
			}
		}
	}
	else
	{
		m_file = file;
	}

	// Utilize the Default entity class
	return CEntityObject::Init(prev, "Entity");
}

bool CEntityObjectWithAnimatedMeshComponent::CreateGameObject()
{
	if (!CEntityObject::CreateGameObject())
	{
		return false;
	}

	if (CAnimatedMeshComponent* pMeshComponent = m_pEntity->GetOrCreateComponent<CAnimatedMeshComponent>())
	{
		if (m_file.size() > 0)
		{
			pMeshComponent->SetCharacterFile(m_file.c_str());
			pMeshComponent->LoadFromDisk();
			pMeshComponent->ResetObject();
		}

		pMeshComponent->GetComponentFlags().Add(EEntityComponentFlags::UserAdded);
	}

	// Physicalize the object by default
	if (CRigidBodyComponent* pPhysicsComponent = m_pEntity->GetOrCreateComponent<CRigidBodyComponent>())
	{
		pPhysicsComponent->GetComponentFlags().Add(EEntityComponentFlags::UserAdded);
	}

	return true;
}
