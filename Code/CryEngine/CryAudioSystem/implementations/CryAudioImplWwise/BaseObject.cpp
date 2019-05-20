// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"
#include "Event.h"
#include "EventInstance.h"
#include "Impl.h"
#include "Listener.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject(
	AkGameObjectID const id,
	ListenerInfos const& listenerInfos,
	char const* const szName /*= nullptr*/,
	Vec3 const& position /*= { 0.0f, 0.0f, 0.0f }*/)
	: m_id(id)
	, m_flags(EObjectFlags::None)
	, m_position(position)
	, m_shortestDistanceToListener(0.0f)
	, m_listenerInfos(listenerInfos)
#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	, m_name(szName)
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
{
	m_eventInstances.reserve(2);
	SetListeners();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Update(float const deltaTime)
{
	SetDistanceToListener();

	EObjectFlags const previousFlags = m_flags;

	if (!m_eventInstances.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;
	}

	auto iter(m_eventInstances.begin());
	auto iterEnd(m_eventInstances.end());

	while (iter != iterEnd)
	{
		CEventInstance* const pEventInstance = *iter;

		if (pEventInstance->IsToBeRemoved())
		{
			gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEventInstance->GetTriggerInstanceId(), ETriggerResult::Playing);
			g_pImpl->DestructEventInstance(pEventInstance);

			if (iter != (iterEnd - 1))
			{
				(*iter) = m_eventInstances.back();
			}

			m_eventInstances.pop_back();
			iter = m_eventInstances.begin();
			iterEnd = m_eventInstances.end();
		}
		else
		{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
			// Always update in production code for debug draw.
			pEventInstance->UpdateVirtualState(m_shortestDistanceToListener);

			if (pEventInstance->GetState() != EEventInstanceState::Virtual)
			{
				m_flags &= ~EObjectFlags::IsVirtual;
			}
#else
			if (((m_flags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::UpdateVirtualStates) != 0))
			{
				pEventInstance->UpdateVirtualState(m_shortestDistanceToListener);

				if (pEventInstance->GetState() != EEventInstanceState::Virtual)
				{
					m_flags &= ~EObjectFlags::IsVirtual;
				}
			}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
			++iter;
		}
	}

	if ((previousFlags != m_flags) && !m_eventInstances.empty())
	{
		if (((previousFlags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::IsVirtual) == 0))
		{
			gEnv->pAudioSystem->ReportPhysicalizedObject(this);
		}
		else if (((previousFlags& EObjectFlags::IsVirtual) == 0) && ((m_flags& EObjectFlags::IsVirtual) != 0))
		{
			gEnv->pAudioSystem->ReportVirtualizedObject(this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddEventInstance(CEventInstance* const pEventInstance)
{
	SetDistanceToListener();

	pEventInstance->UpdateVirtualState(m_shortestDistanceToListener);

	if ((m_flags& EObjectFlags::IsVirtual) != 0)
	{
		if (pEventInstance->GetState() != EEventInstanceState::Virtual)
		{
			m_flags &= ~EObjectFlags::IsVirtual;
		}
	}
	else if (m_eventInstances.empty())
	{
		if (pEventInstance->GetState() == EEventInstanceState::Virtual)
		{
			m_flags |= EObjectFlags::IsVirtual;
		}
	}

	m_eventInstances.push_back(pEventInstance);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopAllTriggers()
{
	AK::SoundEngine::StopAll(m_id);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::SetName(char const* const szName)
{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	StopAllTriggers();

	AKRESULT wwiseResult = AK::SoundEngine::UnregisterGameObj(m_id);
	CRY_ASSERT(wwiseResult == AK_Success);

	wwiseResult = AK::SoundEngine::RegisterGameObj(m_id, szName);
	CRY_ASSERT(wwiseResult == AK_Success);

	m_name = szName;

	return ERequestStatus::SuccessNeedsRefresh;
#else
	return ERequestStatus::Success;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddListener(IListener* const pIListener)
{
	auto const pListener = static_cast<CListener*>(pIListener);
	float const distance = m_position.GetDistance(pListener->GetPosition());

	m_listenerInfos.emplace_back(pListener, distance);

	SetListeners();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveListener(IListener* const pIListener)
{
	auto const pListener = static_cast<CListener*>(pIListener);
	bool wasRemoved = false;

	auto iter(m_listenerInfos.begin());
	auto const iterEnd(m_listenerInfos.cend());

	for (; iter != iterEnd; ++iter)
	{
		SListenerInfo const& info = *iter;

		if (info.pListener == pListener)
		{
			if (iter != (iterEnd - 1))
			{
				(*iter) = m_listenerInfos.back();
			}

			m_listenerInfos.pop_back();
			wasRemoved = true;
			break;
		}
	}

	if (wasRemoved)
	{
		SetListeners();
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopEvent(AkUniqueID const eventId)
{
	for (auto const pEventInstance : m_eventInstances)
	{
		if (pEventInstance->GetEvent().GetId() == eventId)
		{
			pEventInstance->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetDistanceToListener()
{
	m_shortestDistanceToListener = std::numeric_limits<float>::max();

	for (auto& listenerInfo : m_listenerInfos)
	{
		listenerInfo.distance = m_position.GetDistance(listenerInfo.pListener->GetPosition());
		m_shortestDistanceToListener = std::min(m_shortestDistanceToListener, listenerInfo.distance);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetListeners()
{
	size_t const numListeners = m_listenerInfos.size();
	AkGameObjectID* const listenerIds = new AkGameObjectID[numListeners];

	for (size_t i = 0; i < numListeners; ++i)
	{
		AkGameObjectID const id = m_listenerInfos[i].pListener->GetId();
		listenerIds[i] = id;
	}

	AK::SoundEngine::SetListeners(m_id, listenerIds, static_cast<AkUInt32>(numListeners));
	delete[] listenerIds;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	UpdateListenerNames();
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateListenerNames()
{
	m_listenerNames.clear();
	size_t const numListeners = m_listenerInfos.size();

	if (numListeners != 0)
	{
		for (size_t i = 0; i < numListeners; ++i)
		{
			m_listenerNames += m_listenerInfos[i].pListener->GetName();

			if (i != (numListeners - 1))
			{
				m_listenerNames += ", ";
			}
		}
	}
	else
	{
		m_listenerNames = "No Listener!";
	}
}
#endif // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
}      // namespace Wwise
}      // namespace Impl
}      // namespace CryAudio
