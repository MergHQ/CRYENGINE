// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "Event.h"
#include "EventInstance.h"
#include "Impl.h"

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
void CObject::StopEvent(uint32 const pathId)
{
	for (auto const pEventInstance : m_eventInstances)
	{
		if (pEventInstance->GetPathId() == pathId)
		{
			pEventInstance->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::RegisterEventInstance(CEventInstance* const pEventInstance)
{
	m_eventInstances.push_back(pEventInstance);
}

//////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
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
			pEventInstance->Update();
			++iter;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetTransformation(CTransformation const& transformation)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusion(float const occlusion)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionType(EOcclusionType const occlusionType)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::StopAllTriggers()
{
	for (auto const pEventInstance : m_eventInstances)
	{
		pEventInstance->Stop();
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	m_name = szName;
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
