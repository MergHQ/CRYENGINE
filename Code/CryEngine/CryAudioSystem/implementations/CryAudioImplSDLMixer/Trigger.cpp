// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
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
ERequestStatus CTrigger::Execute(IObject* const pIObject, IEvent* const pIEvent)
{
	if ((pIObject != nullptr) && (pIEvent != nullptr))
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		auto const pEvent = static_cast<CEvent*>(pIEvent);
		return SoundEngine::ExecuteEvent(pObject, this, pEvent);
	}

	return ERequestStatus::Failure;
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
