// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   LipSync_FacialInstance.cpp
//  Version:     v1.00
//  Created:     2014-08-29 by Christian Werle.
//  Description: Automatic start of facial animation when a sound is being played back.
//               Legacy version that uses CryAnimation's FacialInstance.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "LipSync_FacialInstance.h"

//=============================================================================
//
// CLipSyncProvider_FacialInstance
//
//=============================================================================

CLipSyncProvider_FacialInstance::CLipSyncProvider_FacialInstance(EntityId entityId)
	: m_entityId(entityId)
{
}

void CLipSyncProvider_FacialInstance::RequestLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod)
{
	// actually facial sequence is triggered in OnSoundEvent SOUND_EVENT_ON_PLAYBACK_STARTED of the CSoundProxy
	// when playback is started, it will start facial sequence as well
}

void CLipSyncProvider_FacialInstance::StartLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod)
{
	if (lipSyncMethod != eLSM_None)
	{
		LipSyncWithSound(audioTriggerId);
	}
}

void CLipSyncProvider_FacialInstance::PauseLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod)
{
	if (lipSyncMethod != eLSM_None)
	{
		LipSyncWithSound(audioTriggerId, true);
	}
}

void CLipSyncProvider_FacialInstance::UnpauseLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod)
{
	if (lipSyncMethod != eLSM_None)
	{
		LipSyncWithSound(audioTriggerId);
	}
}

void CLipSyncProvider_FacialInstance::StopLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod)
{
	if (lipSyncMethod != eLSM_None)
	{
		LipSyncWithSound(audioTriggerId, true);
	}
}

void CLipSyncProvider_FacialInstance::UpdateLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod)
{
}

void CLipSyncProvider_FacialInstance::FullSerialize(TSerialize ser)
{
	ser.BeginGroup("LipSyncProvider_FacialInstance");
	ser.EndGroup();
}

void CLipSyncProvider_FacialInstance::GetEntityPoolSignature(TSerialize signature)
{
	signature.BeginGroup("LipSyncProvider_FacialInstance");
	signature.EndGroup();
}

void CLipSyncProvider_FacialInstance::LipSyncWithSound(const AudioControlId audioTriggerId, bool bStop /*= false*/)
{
	if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		if (ICharacterInstance* pChar = pEntity->GetCharacter(0))
		{
			if (IFacialInstance* pFacial = pChar->GetFacialInstance())
			{
				pFacial->LipSyncWithSound(audioTriggerId, bStop);
			}
		}
	}
}

//=============================================================================
//
// CLipSync_FacialInstance
//
//=============================================================================

void CLipSync_FacialInstance::InjectLipSyncProvider()
{
	IEntity* pEntity = GetEntity();
	IEntityAudioProxy* pSoundProxy = static_cast<IEntityAudioProxy*>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO).get());
	CRY_ASSERT(pSoundProxy);
	m_pLipSyncProvider.reset(new CLipSyncProvider_FacialInstance(pEntity->GetId()));
	REINST(add SetLipSyncProvider to interface)
	//pSoundProxy->SetLipSyncProvider(m_pLipSyncProvider);
}

void CLipSync_FacialInstance::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->Add(*this);
	if (m_pLipSyncProvider)
	{
		pSizer->Add(*m_pLipSyncProvider);
	}
}

bool CLipSync_FacialInstance::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);
	return true;
}

void CLipSync_FacialInstance::PostInit(IGameObject* pGameObject)
{
	InjectLipSyncProvider();
}

void CLipSync_FacialInstance::InitClient(int channelId)
{
}

void CLipSync_FacialInstance::PostInitClient(int channelId)
{
}

bool CLipSync_FacialInstance::ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
	ResetGameObject();
	return true;
}

void CLipSync_FacialInstance::PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
	InjectLipSyncProvider();
}

bool CLipSync_FacialInstance::GetEntityPoolSignature(TSerialize signature)
{
	signature.BeginGroup("LipSync_FacialInstance");
	if (m_pLipSyncProvider)
	{
		m_pLipSyncProvider->GetEntityPoolSignature(signature);
	}
	signature.EndGroup();
	return true;
}

void CLipSync_FacialInstance::Release()
{
	IEntity* pEntity = GetEntity();
	if (IEntityAudioProxy* pSoundProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO)))
	{
		REINST(add SetLipSyncProvider to interface)
		//pSoundProxy->SetLipSyncProvider(ILipSyncProviderPtr());
	}
	delete this;
}

void CLipSync_FacialInstance::FullSerialize(TSerialize ser)
{
	ser.BeginGroup("LipSync_FacialInstance");

	bool bLipSyncProviderIsInjected = (m_pLipSyncProvider != NULL);
	ser.Value("bLipSyncProviderIsInjected", bLipSyncProviderIsInjected);
	if (bLipSyncProviderIsInjected && !m_pLipSyncProvider)
	{
		CRY_ASSERT(ser.IsReading());
		InjectLipSyncProvider();
	}
	if (m_pLipSyncProvider)
	{
		m_pLipSyncProvider->FullSerialize(ser);
	}

	ser.EndGroup();
}

bool CLipSync_FacialInstance::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags)
{
	return true;
}

void CLipSync_FacialInstance::PostSerialize()
{
}

void CLipSync_FacialInstance::SerializeSpawnInfo(TSerialize ser)
{
}

ISerializableInfoPtr CLipSync_FacialInstance::GetSpawnInfo()
{
	return NULL;
}

void CLipSync_FacialInstance::Update(SEntityUpdateContext& ctx, int updateSlot)
{
}

void CLipSync_FacialInstance::HandleEvent(const SGameObjectEvent& event)
{
}

void CLipSync_FacialInstance::ProcessEvent(SEntityEvent& event)
{
}

void CLipSync_FacialInstance::SetChannelId(uint16 id)
{
}

void CLipSync_FacialInstance::SetAuthority(bool auth)
{
}

void CLipSync_FacialInstance::PostUpdate(float frameTime)
{
}

void CLipSync_FacialInstance::PostRemoteSpawn()
{
}
