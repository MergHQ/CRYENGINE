// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "Impl.h"
#include "Trigger.h"
#include "Object.h"
#include "Event.h"
#include "SoundEngine.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus status = ERequestStatus::Failure;

	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		status = SoundEngine::ExecuteTrigger(pObject, this, triggerInstanceId);
	}

	return status;
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Stop(IObject* const pIObject)
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
