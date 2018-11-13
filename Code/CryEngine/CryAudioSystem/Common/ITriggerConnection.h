// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IEvent;

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
	 * Load the metadata and media needed by the audio middleware to execute this trigger
	 * Loading Triggers manually is only necessary if their data have not been loaded via PreloadRequests
	 * @return ERequestStatus::Success if the the data was successfully loaded, ERequestStatus::Failure otherwise
	 * @see Unload, LoadAsync, UnloadAsync
	 */
	virtual ERequestStatus Load() const = 0;

	/**
	 * Release the metadata and media needed by the audio middleware to execute this trigger
	 * Unloading Triggers manually is only necessary if their data are not managed via PreloadRequests.
	 * @return ERequestStatus::Success if the the data was successfully unloaded, ERequestStatus::Failure otherwise
	 * @see Load, LoadAsync, UnloadAsync
	 */
	virtual ERequestStatus Unload() const = 0;

	/**
	 * Load the metadata and media needed by the audio middleware to execute this trigger asynchronously.
	 * Loading Triggers manually is only necessary if their data have not been loaded via PreloadRequests.
	 * @param pIEvent - The callback called once the loading is done must report that this event is finished.
	 * @return ERequestStatus::Success if the the request was successfully sent to the audio middleware, ERequestStatus::Failure otherwise
	 * @see Load, Unload, UnloadAsync
	 */
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const = 0;

	/**
	 * Release the metadata and media needed by the audio middleware to execute this trigger asynchronously.
	 * Unloading Triggers manually is only necessary if their data have not been loaded via PreloadRequests.
	 * @param pIEvent - The callback called once the loading is done must report that this event is finished.
	 * @return ERequestStatus::Success if the the request was successfully sent to the audio middleware, ERequestStatus::Failure otherwise
	 * @see Load, Unload, LoadAsync
	 */
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const = 0;
};
} // namespace Impl
} // namespace CryAudio
