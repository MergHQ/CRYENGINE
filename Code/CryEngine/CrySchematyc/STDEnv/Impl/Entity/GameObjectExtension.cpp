// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GameObjectExtension.h"

#include <Schematyc/Entity/EntityUserData.h>

namespace Schematyc
{
bool CGameObjectExtension::RegisterSerializeCallback(int32 aspects, const NetworkSerializeCallback& callback, CConnectionScope& connectionScope)
{
	return false;
}

void CGameObjectExtension::MarkAspectsDirty(int32 aspects) {}

bool CGameObjectExtension::ClientAuthority() const
{
	return false;
}

uint16 CGameObjectExtension::GetChannelId() const
{
	return 0;
}

bool CGameObjectExtension::ServerAuthority() const
{
	return true;
}

bool CGameObjectExtension::LocalAuthority() const
{
	return gEnv->bServer;
}

bool CGameObjectExtension::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);

	bool bIsPreview = false;
	if (gEnv->IsEditor())
	{
		const SEntityUserData* pUserData = static_cast<const SEntityUserData*>(pGameObject->GetUserData());
		if (pUserData)
		{
			bIsPreview = pUserData->bIsPreview;
		}
	}

	return m_entityObject.Init(GetEntity(), bIsPreview);
}

void CGameObjectExtension::InitClient(int channelId) {}

void CGameObjectExtension::PostInit(IGameObject* pGameObject)
{
	m_entityObject.PostInit();
}

void CGameObjectExtension::PostInitClient(int channelId) {}

bool CGameObjectExtension::ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& spawnParams)
{
	return false;
}

void CGameObjectExtension::PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& spawnParams) {}

void CGameObjectExtension::Release()
{
	delete this;
}

void CGameObjectExtension::FullSerialize(TSerialize serialize) {}

bool CGameObjectExtension::NetSerialize(TSerialize serialize, EEntityAspects aspects, uint8 profile, int flags)
{
	return true;
}

void                 CGameObjectExtension::PostSerialize()                          {}

void                 CGameObjectExtension::SerializeSpawnInfo(TSerialize serialize) {}

ISerializableInfoPtr CGameObjectExtension::GetSpawnInfo()
{
	return ISerializableInfoPtr();
}

void CGameObjectExtension::Update(SEntityUpdateContext& context, int updateSlot) {}

void CGameObjectExtension::PostUpdate(float frameTime)                           {}

void CGameObjectExtension::PostRemoteSpawn()                                     {}

void CGameObjectExtension::ProcessEvent(SEntityEvent& event)
{
	m_entityObject.ProcessEvent(event);
}

void CGameObjectExtension::HandleEvent(const SGameObjectEvent& event) {}

void CGameObjectExtension::SetChannelId(uint16 channelId)             {}

void CGameObjectExtension::SetAuthority(bool bLocalAuthority)         {}

void CGameObjectExtension::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->Add(*this);
}

const char* CGameObjectExtension::ms_szExtensionName = "SchematycGameObjectExtension";
} // Schematyc
