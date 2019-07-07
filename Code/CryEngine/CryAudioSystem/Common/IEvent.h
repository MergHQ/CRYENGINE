// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IEvent
 * (e.g. a middleware-specific playingID of an active event/sound for a play event)
 */
struct IEvent
{
	/** @cond */
	virtual ~IEvent() = default;
	/** @endcond */

	/**
	 * Stop the playing event
	 * @return ERequestStatus::Success if the event is stopped, ERequestStatus::Failure otherwise
	 */
	virtual ERequestStatus Stop() = 0;
};
} // namespace Impl
} // namespace CryAudio
