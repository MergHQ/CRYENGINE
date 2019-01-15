// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "Event.h"
#include "Impl.h"
#include "Trigger.h"
#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
void CObject::StopEvent(uint32 const pathId)
{
	for (auto const pEvent : m_activeEvents)
	{
		if (pEvent->pathId == pathId)
		{
			pEvent->Stop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::RegisterEvent(CEvent* const pEvent)
{
	m_activeEvents.push_back(pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
	auto iter(m_activeEvents.begin());
	auto iterEnd(m_activeEvents.end());

	while (iter != iterEnd)
	{
		auto const pEvent = *iter;

		if (pEvent->toBeRemoved)
		{
			gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEvent->GetTriggerInstanceId());
			g_pImpl->DestructEvent(pEvent);

			if (iter != (iterEnd - 1))
			{
				(*iter) = m_activeEvents.back();
			}

			m_activeEvents.pop_back();
			iter = m_activeEvents.begin();
			iterEnd = m_activeEvents.end();
		}
		else
		{
			pEvent->Update();
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
	for (auto const pEvent : m_activeEvents)
	{
		pEvent->Stop();
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	m_name = szName;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

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
