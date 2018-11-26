// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Environment.h"
#include "Common.h"
#include "Object.h"
#include "Event.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
void CEnvironment::Set(IObject* const pIObject, float const amount)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CBaseObject*>(pIObject);
		bool shouldUpdate = true;

		Environments& objectEnvironments = pObject->GetEnvironments();
		auto const iter(objectEnvironments.find(this));

		if (iter != objectEnvironments.end())
		{
			if (shouldUpdate = (fabs(iter->second - amount) > 0.001f))
			{
				iter->second = amount;
			}
		}
		else
		{
			objectEnvironments.emplace(this, amount);
		}

		if (shouldUpdate)
		{
			Events const& objectEvents = pObject->GetEvents();

			for (auto const pEvent : objectEvents)
			{
				pEvent->TrySetEnvironment(this, amount);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object pointer passed to the Fmod implementation of %s", __FUNCTION__);
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio