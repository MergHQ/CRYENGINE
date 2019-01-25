// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"
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
	char const* const szName /*= nullptr*/,
	Vec3 const position /*= { 0.0f, 0.0f, 0.0f }*/)
	: m_id(id)
	, m_flags(EObjectFlags::None)
	, m_position(position)
	, m_distanceToListener(0.0f)
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	, m_name(szName)
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
{
	m_eventInstances.reserve(2);
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
		auto const pEventInstance = *iter;

		if (pEventInstance->IsToBeRemoved())
		{
			gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEventInstance->GetTriggerInstanceId());
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
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
			// Always update in production code for debug draw.
			pEventInstance->UpdateVirtualState(m_distanceToListener);

			if (pEventInstance->GetState() != EEventInstanceState::Virtual)
			{
				m_flags &= ~EObjectFlags::IsVirtual;
			}
#else
			if (((m_flags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::UpdateVirtualStates) != 0))
			{
				pEventInstance->UpdateVirtualState(m_distanceToListener);

				if (pEventInstance->GetState() != EEventInstanceState::Virtual)
				{
					m_flags &= ~EObjectFlags::IsVirtual;
				}
			}
#endif      // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
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

	pEventInstance->UpdateVirtualState(m_distanceToListener);

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
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	StopAllTriggers();

	AKRESULT wwiseResult = AK::SoundEngine::UnregisterGameObj(m_id);
	CRY_ASSERT(wwiseResult == AK_Success);

	wwiseResult = AK::SoundEngine::RegisterGameObj(m_id, szName);
	CRY_ASSERT(wwiseResult == AK_Success);

	m_name = szName;

	return ERequestStatus::SuccessNeedsRefresh;
#else
	return ERequestStatus::Success;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopEvent(AkUniqueID const eventId)
{
	for (auto const pEventInstance : m_eventInstances)
	{
		if (pEventInstance->GetEventId() == eventId)
		{
			pEventInstance->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetDistanceToListener()
{
	m_distanceToListener = m_position.GetDistance(g_pListener->GetPosition());
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
