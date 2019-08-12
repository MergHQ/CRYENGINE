// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
struct STriggerCallbackData;

namespace Impl
{
struct IObject;

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding ITrigger
 * (e.g. a middleware-specific event ID or name, a sound-file name to be passed to an API function)
 */
struct ITriggerConnection
{
	/** @cond */
	virtual ~ITriggerConnection() = default;
	/** @endcond */

	/**
	 * Activate a trigger on this object.
	 * @param pIObject - implementation-specific object to execute the trigger on.The audio system guarantees that this is never a null pointer.
	 * @param triggerInstanceId - instance id of the executed trigger.
	 * @return ETriggerResult - indicates the outcome of underlying process.
	 * @see Stop
	 */
	virtual ETriggerResult Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) = 0;

	/**
	 * Activate a trigger on this object and registers for receiving callbacks.
	 * @param pIObject - implementation-specific object to execute the trigger on.The audio system guarantees that this is never a null pointer.
	 * @param triggerInstanceId - instance id of the executed trigger.
	 * @param callbackData - struct to pass data for callbacks.
	 * @return ETriggerResult - indicates the outcome of underlying process.
	 * @see Stop
	 */
	virtual ETriggerResult ExecuteWithCallbacks(IObject* const pIObject, TriggerInstanceId const triggerInstanceId, STriggerCallbackData const& callbackData) = 0;

	/**
	 * Stops a trigger on this object.
	 * @param pIObject - implementation-specific object to stop the trigger on. The audio system guarantees that this is never a null pointer.
	 * @return void
	 * @see Execute, ExecuteWithCallbacks
	 */
	virtual void Stop(IObject* const pIObject) = 0;
};
} // namespace Impl
} // namespace CryAudio
