// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EntityAudioProxy.h"
#include "Entity.h"
#include <CryAudio/IAudioSystem.h>

using namespace CryAudio;

CRYREGISTER_CLASS(CEntityComponentAudio);

CEntityComponentAudio::AudioAuxObjectPair CEntityComponentAudio::s_nullAudioProxyPair(InvalidAuxObjectId, static_cast<IObject*>(nullptr));
CObjectTransformation CEntityComponentAudio::s_audioListenerLastTransformation;

//////////////////////////////////////////////////////////////////////////
CEntityComponentAudio::CEntityComponentAudio()
	: m_audioAuxObjectIdCounter(InvalidAuxObjectId)
	, m_audioEnvironmentId(InvalidEnvironmentId)
	, m_flags(eEntityAudioProxyFlags_CanMoveWithEntity)
	, m_pIListener(nullptr)
	, m_fadeDistance(0.0f)
	, m_environmentFadeDistance(0.0f)
{
}

//////////////////////////////////////////////////////////////////////////
CEntityComponentAudio::~CEntityComponentAudio()
{
	std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SReleaseAudioProxy());
	m_mapAuxAudioProxies.clear();

	if (m_pIListener != nullptr)
	{
		gEnv->pAudioSystem->ReleaseListener(m_pIListener);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::Initialize()
{
	assert(m_mapAuxAudioProxies.empty());

	if ((m_pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)
	{
		m_flags &= ~eEntityAudioProxyFlags_CanMoveWithEntity;
		m_pIListener = gEnv->pAudioSystem->CreateListener();

		Matrix34 const& tm = m_pEntity->GetWorldTM();
		CRY_ASSERT_MESSAGE(tm.IsValid(), "Invalid Matrix34 during CEntityComponentAudio::Initialize");
		Matrix34 transformation = tm;
		transformation += CVar::audioListenerOffset;
		s_audioListenerLastTransformation = transformation;
		if (m_pIListener != nullptr)
		{
			m_pIListener->SetTransformation(s_audioListenerLastTransformation);
		}
	}

	// Creating the default AudioProxy.
	CreateAudioAuxObject();
	OnMove();
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::OnMove()
{
	CRY_ASSERT_MESSAGE(!(((m_flags & eEntityAudioProxyFlags_CanMoveWithEntity) > 0) && ((m_pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)), "An CEntityAudioProxy cannot have both flags (eEAPF_CAN_MOVE_WITH_ENTITY & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) set simultaneously!");

	Matrix34 const& tm = m_pEntity->GetWorldTM();
	CRY_ASSERT_MESSAGE(tm.IsValid(), "Invalid Matrix34 during CEntityComponentAudio::OnMove");

	if ((m_flags & eEntityAudioProxyFlags_CanMoveWithEntity) > 0)
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SRepositionAudioProxy(tm, SRequestUserData::GetEmptyObject()));
	}
	else if ((m_pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)
	{
		Matrix34 transformation = tm;
		transformation += CVar::audioListenerOffset;

		if (!s_audioListenerLastTransformation.IsEquivalent(transformation, 0.01f))
		{
			s_audioListenerLastTransformation = transformation;
			if (m_pIListener != nullptr)
			{
				m_pIListener->SetTransformation(s_audioListenerLastTransformation);
			}

			// As this is an audio listener add its entity to the AreaManager for raising audio relevant events.
			gEnv->pEntitySystem->GetAreaManager()->MarkEntityForUpdate(m_pEntity->GetId());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::OnListenerMoveInside(Vec3 const& listenerPos)
{
	m_pEntity->SetPos(listenerPos);
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::OnListenerExclusiveMoveInside(IEntity const* const __restrict pEntity, IEntity const* const __restrict pAreaHigh, IEntity const* const __restrict pAreaLow, float const fade)
{
	IEntityAreaComponent const* const __restrict pAreaProxyLow = static_cast<IEntityAreaComponent const* const __restrict>(pAreaLow->GetProxy(ENTITY_PROXY_AREA));
	IEntityAreaComponent* const __restrict pAreaProxyHigh = static_cast<IEntityAreaComponent* const __restrict>(pAreaHigh->GetProxy(ENTITY_PROXY_AREA));

	if (pAreaProxyLow != nullptr && pAreaProxyHigh != nullptr)
	{
		Vec3 onHighHull3d(ZERO);
		Vec3 const pos(pEntity->GetWorldPos());
		EntityId const entityId = pEntity->GetId();
		bool const bInsideLow = pAreaProxyLow->CalcPointWithin(entityId, pos);

		if (bInsideLow)
		{
			pAreaProxyHigh->ClosestPointOnHullDistSq(entityId, pos, onHighHull3d);
			m_pEntity->SetPos(onHighHull3d);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::OnListenerEnter(IEntity const* const pEntity)
{
	m_pEntity->SetPos(pEntity->GetWorldPos());
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::OnListenerMoveNear(Vec3 const& closestPointToArea)
{
	m_pEntity->SetPos(closestPointToArea);
}

//////////////////////////////////////////////////////////////////////////
uint64 CEntityComponentAudio::GetEventMask() const
{
	return
	  BIT64(ENTITY_EVENT_DONE) |
	  BIT64(ENTITY_EVENT_XFORM) |
	  BIT64(ENTITY_EVENT_ENTERAREA) |
	  BIT64(ENTITY_EVENT_MOVENEARAREA) |
	  BIT64(ENTITY_EVENT_ENTERNEARAREA) |
	  BIT64(ENTITY_EVENT_MOVEINSIDEAREA) |
	  BIT64(ENTITY_EVENT_ANIM_EVENT);
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::ProcessEvent(SEntityEvent& event)
{
	if (m_pEntity != nullptr)
	{
		switch (event.event)
		{
		case ENTITY_EVENT_XFORM:
			{
				int const flags = (int)event.nParam[0];

				if ((flags & (ENTITY_XFORM_POS | ENTITY_XFORM_ROT)) > 0)
				{
					OnMove();
				}

				break;
			}
		case ENTITY_EVENT_ENTERAREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) > 0)
				{
					EntityId const entityId = static_cast<EntityId>(event.nParam[0]); // Entering entity!
					IEntity* const pIEntity = gEnv->pEntitySystem->GetEntity(entityId);

					if ((pIEntity != nullptr) && (pIEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)
					{
						OnListenerEnter(pIEntity);
					}
				}

				break;
			}
		case ENTITY_EVENT_MOVENEARAREA:
		case ENTITY_EVENT_ENTERNEARAREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) > 0)
				{
					EntityId const entityId = static_cast<EntityId>(event.nParam[0]); // Near entering/moving entity!
					IEntity* const pIEntity = gEnv->pEntitySystem->GetEntity(entityId);

					if (pIEntity != nullptr)
					{
						if ((pIEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)
						{
							OnListenerMoveNear(event.vec);
						}
					}
				}

				break;
			}
		case ENTITY_EVENT_MOVEINSIDEAREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) > 0)
				{
					EntityId const entityId = static_cast<EntityId>(event.nParam[0]); // Inside moving entity!
					IEntity* const __restrict pIEntity = gEnv->pEntitySystem->GetEntity(entityId);

					if (pIEntity != nullptr)
					{
						EntityId const area1Id = static_cast<EntityId>(event.nParam[2]); // AreaEntityID (low)
						EntityId const area2Id = static_cast<EntityId>(event.nParam[3]); // AreaEntityID (high)

						IEntity* const __restrict pArea1 = gEnv->pEntitySystem->GetEntity(area1Id);
						IEntity* const __restrict pArea2 = gEnv->pEntitySystem->GetEntity(area2Id);

						if (pArea1 != nullptr)
						{
							if (pArea2 != nullptr)
							{
								if ((pIEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)
								{
									OnListenerExclusiveMoveInside(pIEntity, pArea2, pArea1, event.fParam[0]);
								}
							}
							else
							{
								if ((pIEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) > 0)
								{
									OnListenerMoveInside(event.vec);
								}
							}
						}
					}
				}

				break;
			}
		case ENTITY_EVENT_ANIM_EVENT:
			{
				REINST("reintroduce anim event voice playing in EntityAudioProxy");
				/*if (!IsSoundAnimEventsHandledExternally())
				   {
				   const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
				   ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
				   const char* eventName = (pAnimEvent ? pAnimEvent->m_EventName : 0);
				   if (eventName && stricmp(eventName, "sound") == 0)
				   {
				    Vec3 offset(ZERO);
				    if (pAnimEvent->m_BonePathName && pAnimEvent->m_BonePathName[0])
				    {
				      if (pCharacter)
				      {
				        IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();
				        int id = rIDefaultSkeleton.GetJointIDByName(pAnimEvent->m_BonePathName);
				        if (id >= 0)
				        {
				          ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
				          QuatT boneQuat(pSkeletonPose->GetAbsJointByID(id));
				          offset = boneQuat.t;
				        }
				      }
				    }

				    int flags = FLAG_SOUND_DEFAULT_3D;
				    if (strchr(pAnimEvent->m_CustomParameter, ':') == nullptr)
				      flags |= FLAG_SOUND_VOICE;
				    PlaySound(pAnimEvent->m_CustomParameter, offset, FORWARD_DIRECTION, flags, 0, eSoundSemantic_Animation, 0, 0);
				   }
				   }*/

				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::GameSerialize(TSerialize ser)
{
}

//////////////////////////////////////////////////////////////////////////
bool CEntityComponentAudio::PlayFile(SPlayFileInfo const& playbackInfo, AuxObjectId const audioAuxObjectId /* = DefaultAuxObjectId */, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (m_pEntity != nullptr)
	{
		if (audioAuxObjectId != InvalidAuxObjectId)
		{
			AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

			if (audioObjectPair.first != InvalidAuxObjectId)
			{
				(SPlayFile(playbackInfo, userData))(audioObjectPair);
				return true;
			}
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
			else
			{
				gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Could not find AuxAudioProxy with id '%u' on entity '%s' to PlayFile '%s'", audioAuxObjectId, m_pEntity->GetEntityTextDescription().c_str(), playbackInfo.szFile);
			}
#endif  // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
		}
		else
		{
			std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SPlayFile(playbackInfo, userData));
			return !m_mapAuxAudioProxies.empty();
		}
	}
	else
	{
		gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to play an audio file on an EntityAudioProxy without a valid entity!");
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::StopFile(
  char const* const szFile,
  AuxObjectId const audioAuxObjectId /*= DefaultAuxObjectId*/)
{
	if (m_pEntity != nullptr)
	{
		if (audioAuxObjectId != InvalidAuxObjectId)
		{
			AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

			if (audioObjectPair.first != InvalidAuxObjectId)
			{
				(SStopFile(szFile))(audioObjectPair);
			}
		}
		else
		{
			std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SStopFile(szFile));
		}
	}
	else
	{
		gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to stop an audio file on an EntityAudioProxy without a valid entity!");
	}
}

//////////////////////////////////////////////////////////////////////////
bool CEntityComponentAudio::ExecuteTrigger(
  ControlId const audioTriggerId,
  AuxObjectId const audioAuxObjectId /* = DefaultAuxObjectId */,
  SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (m_pEntity != nullptr)
	{
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
		if (m_pEntity->GetWorldTM().GetTranslation() == Vec3Constants<float>::fVec3_Zero)
		{
			gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger at (0,0,0) position in the entity %s. Entity may not be initialized correctly!", m_pEntity->GetEntityTextDescription().c_str());
		}
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

		if ((m_pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_DISABLED) == 0)
		{
			if (audioAuxObjectId != InvalidAuxObjectId)
			{
				AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

				if (audioObjectPair.first != InvalidAuxObjectId)
				{
					(SRepositionAudioProxy(m_pEntity->GetWorldTM(), userData))(audioObjectPair);
					audioObjectPair.second.pIObject->ExecuteTrigger(audioTriggerId, userData);
					return true;
				}
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
				else
				{
					gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Could not find AuxAudioProxy with id '%u' on entity '%s' to ExecuteTrigger '%u'", audioAuxObjectId, m_pEntity->GetEntityTextDescription().c_str(), audioTriggerId);
				}
#endif  // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
			}
			else
			{
				for (AudioAuxObjects::iterator it = m_mapAuxAudioProxies.begin(); it != m_mapAuxAudioProxies.end(); ++it)
				{
					(SRepositionAudioProxy(m_pEntity->GetWorldTM(), userData))(*it);
					it->second.pIObject->ExecuteTrigger(audioTriggerId, userData);
				}
				return !m_mapAuxAudioProxies.empty();
			}
		}
	}
	else
	{
		gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger on an EntityAudioProxy without a valid entity!");
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::StopTrigger(
  ControlId const audioTriggerId,
  AuxObjectId const audioAuxObjectId /* = DefaultAuxObjectId */,
  SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	if (audioAuxObjectId != InvalidAuxObjectId)
	{
		AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

		if (audioObjectPair.first != InvalidAuxObjectId)
		{
			(SStopTrigger(audioTriggerId, userData))(audioObjectPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SStopTrigger(audioTriggerId, userData));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::SetSwitchState(ControlId const audioSwitchId, SwitchStateId const audioStateId, AuxObjectId const audioAuxObjectId /*= DEFAULT_AUDIO_PROXY_ID*/)
{
	if (audioAuxObjectId != InvalidAuxObjectId)
	{
		AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

		if (audioObjectPair.first != InvalidAuxObjectId)
		{
			(SSetSwitchState(audioSwitchId, audioStateId))(audioObjectPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetSwitchState(audioSwitchId, audioStateId));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::SetParameter(ControlId const parameterId, float const value, AuxObjectId const audioAuxObjectId /*= DefaultAuxObjectId*/)
{
	if (audioAuxObjectId != InvalidAuxObjectId)
	{
		AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

		if (audioObjectPair.first != InvalidAuxObjectId)
		{
			(SSetParameter(parameterId, value))(audioObjectPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetParameter(parameterId, value));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::SetObstructionCalcType(EOcclusionType const occlusionType, AuxObjectId const audioAuxObjectId /*= DEFAULT_AUDIO_PROXY_ID*/)
{
	if (audioAuxObjectId != InvalidAuxObjectId)
	{
		AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

		if (audioObjectPair.first != InvalidAuxObjectId)
		{
			(SSetOcclusionType(occlusionType))(audioObjectPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetOcclusionType(occlusionType));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::SetEnvironmentAmount(EnvironmentId const audioEnvironmentId, float const amount, AuxObjectId const audioAuxObjectId /*= DEFAULT_AUDIO_PROXY_ID*/)
{
	if (audioAuxObjectId != InvalidAuxObjectId)
	{
		AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

		if (audioObjectPair.first != InvalidAuxObjectId)
		{
			SSetEnvironmentAmount(audioEnvironmentId, amount)(audioObjectPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetEnvironmentAmount(audioEnvironmentId, amount));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::SetCurrentEnvironments(AuxObjectId const audioAuxObjectId /*= DEFAULT_AUDIO_PROXY_ID*/)
{
	if (audioAuxObjectId != InvalidAuxObjectId)
	{
		AudioAuxObjectPair const& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

		if (audioObjectPair.first != InvalidAuxObjectId)
		{
			SSetCurrentEnvironments(m_pEntity->GetId())(audioObjectPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetCurrentEnvironments(m_pEntity->GetId()));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::AudioAuxObjectsMoveWithEntity(bool const bCanMoveWithEntity)
{
	if (bCanMoveWithEntity)
	{
		m_flags |= eEntityAudioProxyFlags_CanMoveWithEntity;
	}
	else
	{
		m_flags &= ~eEntityAudioProxyFlags_CanMoveWithEntity;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::AddAsListenerToAudioAuxObject(AuxObjectId const audioAuxObjectId, void (* func)(SRequestInfo const* const), EnumFlagsType const eventMask)
{
	AudioAuxObjects::const_iterator const iter(m_mapAuxAudioProxies.find(audioAuxObjectId));

	if (iter != m_mapAuxAudioProxies.end())
	{
		gEnv->pAudioSystem->AddRequestListener(func, iter->second.pIObject, eventMask);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::RemoveAsListenerFromAudioAuxObject(AuxObjectId const audioAuxObjectId, void (* func)(SRequestInfo const* const))
{
	AudioAuxObjects::const_iterator const iter(m_mapAuxAudioProxies.find(audioAuxObjectId));

	if (iter != m_mapAuxAudioProxies.end())
	{
		gEnv->pAudioSystem->RemoveRequestListener(func, iter->second.pIObject);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::SetAudioAuxObjectOffset(Matrix34 const& offset, AuxObjectId const audioAuxObjectId /*= DEFAULT_AUDIO_PROXY_ID*/)
{
	if (audioAuxObjectId != InvalidAuxObjectId)
	{
		AudioAuxObjectPair& audioObjectPair = GetAudioAuxObjectPair(audioAuxObjectId);

		if (audioObjectPair.first != InvalidAuxObjectId)
		{
			SSetAuxAudioProxyOffset(offset, m_pEntity->GetWorldTM())(audioObjectPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetAuxAudioProxyOffset(offset, m_pEntity->GetWorldTM()));
	}
}

//////////////////////////////////////////////////////////////////////////
Matrix34 const& CEntityComponentAudio::GetAudioAuxObjectOffset(AuxObjectId const audioAuxObjectId /*= DEFAULT_AUDIO_PROXY_ID*/)
{
	AudioAuxObjects::const_iterator const iter(m_mapAuxAudioProxies.find(audioAuxObjectId));

	if (iter != m_mapAuxAudioProxies.end())
	{
		return iter->second.offset;
	}

	static const Matrix34 identityMatrix(IDENTITY);
	return identityMatrix;
}

//////////////////////////////////////////////////////////////////////////
float CEntityComponentAudio::GetGreatestFadeDistance() const
{
	return std::max<float>(m_fadeDistance, m_environmentFadeDistance);
}

//////////////////////////////////////////////////////////////////////////
AuxObjectId CEntityComponentAudio::CreateAudioAuxObject()
{
	AuxObjectId audioAuxObjectId = InvalidAuxObjectId;
	char const* szName = nullptr;

	if ((m_pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) == 0)
	{
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
		if (m_audioAuxObjectIdCounter == std::numeric_limits<AuxObjectId>::max())
		{
			CryFatalError("<Audio> Exceeded numerical limits during CEntityAudioProxy::CreateAudioProxy!");
		}
		else if (m_pEntity == nullptr)
		{
			CryFatalError("<Audio> nullptr entity pointer during CEntityAudioProxy::CreateAudioProxy!");
		}

		CryFixedStringT<MaxObjectNameLength> sFinalName(m_pEntity->GetName());
		size_t const numAudioAuxObjects = m_mapAuxAudioProxies.size();

		if (numAudioAuxObjects > 0)
		{
			// First AuxAudioProxy is not explicitly identified, it keeps the entity's name.
			// All additionally AuxaudioProxies however are being explicitly identified.
			sFinalName.Format("%s_auxaudioproxy_#%" PRISIZE_T, m_pEntity->GetName(), numAudioAuxObjects + 1);
		}

		szName = sFinalName.c_str();
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

		SCreateObjectData const objectData(szName, eOcclusionType_Ignore, m_pEntity->GetWorldTM(), m_pEntity->GetId(), true);
		IObject* const pIObject = gEnv->pAudioSystem->CreateObject(objectData);
		m_mapAuxAudioProxies.insert(AudioAuxObjectPair(++m_audioAuxObjectIdCounter, SAudioAuxObjectWrapper(pIObject)));
		audioAuxObjectId = m_audioAuxObjectIdCounter;
	}

	return audioAuxObjectId;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityComponentAudio::RemoveAudioAuxObject(AuxObjectId const audioAuxObjectId)
{
	bool bSuccess = false;

	if (audioAuxObjectId != DefaultAuxObjectId)
	{
		AudioAuxObjects::iterator iter(m_mapAuxAudioProxies.find(audioAuxObjectId));

		if (iter != m_mapAuxAudioProxies.end())
		{
			gEnv->pAudioSystem->ReleaseObject(iter->second.pIObject);
			m_mapAuxAudioProxies.erase(iter);
			bSuccess = true;
		}
		else
		{
			gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> AuxAudioProxy with ID '%u' not found during CEntityAudioProxy::RemoveAuxAudioProxy (%s)!", audioAuxObjectId, m_pEntity->GetEntityTextDescription().c_str());
			assert(false);
		}
	}
	else
	{
		gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to remove the default AudioProxy during CEntityAudioProxy::RemoveAuxAudioProxy (%s)!", m_pEntity->GetEntityTextDescription().c_str());
		assert(false);
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
CEntityComponentAudio::AudioAuxObjectPair& CEntityComponentAudio::GetAudioAuxObjectPair(AuxObjectId const audioAuxObjectId)
{
	AudioAuxObjects::iterator const iter(m_mapAuxAudioProxies.find(audioAuxObjectId));

	if (iter != m_mapAuxAudioProxies.end())
	{
		return *iter;
	}

	return s_nullAudioProxyPair;
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentAudio::SetEnvironmentAmountInternal(IEntity const* const pIEntity, float const amount) const
{
	// If the passed-in entity is our parent we skip it.
	// Meaning we do not apply our own environment to ourselves.
	if (pIEntity != nullptr && m_pEntity != nullptr && pIEntity != m_pEntity)
	{
		auto pIEntityAudioComponent = pIEntity->GetComponent<IEntityAudioComponent>();

		if ((pIEntityAudioComponent != nullptr) && (m_audioEnvironmentId != InvalidEnvironmentId))
		{
			// Only set the audio-environment-amount on the entities that already have an AudioProxy.
			// Passing INVALID_AUDIO_PROXY_ID to address all auxiliary AudioProxies on pEntityAudioProxy.
			CRY_ASSERT(amount >= 0.0f && amount <= 1.0f);
			pIEntityAudioComponent->SetEnvironmentAmount(m_audioEnvironmentId, amount, InvalidAuxObjectId);
		}
	}
}
