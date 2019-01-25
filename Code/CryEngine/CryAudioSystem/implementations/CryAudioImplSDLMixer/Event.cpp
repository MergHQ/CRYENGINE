// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"
#include "Common.h"
#include "Impl.h"
#include "Object.h"
#include "EventInstance.h"
#include "SoundEngine.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CEvent::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus status = ERequestStatus::Failure;

	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		status = SoundEngine::ExecuteEvent(pObject, this, triggerInstanceId);
	}

	return status;
}

//////////////////////////////////////////////////////////////////////////
void CEvent::Stop(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		pObject->StopEvent(m_id);
	}
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
