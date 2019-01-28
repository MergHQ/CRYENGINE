// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IObject;

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IStandaloneFile.
 * (e.g. middleware-specific custom data that is associated with the standalone file)
 */
struct IStandaloneFileConnection
{
	/** @cond */
	virtual ~IStandaloneFileConnection() = default;
	/** @endcond */

	/**
	 * Play a stand alone file.
	 * @param pIObject - object to play file on. The audio system guarantees that this is never a null pointer.
	 * @return ERequestStatus - indicates the outcome of underlying process
	 * @see StopFile
	 */
	virtual ERequestStatus Play(IObject* const pIObject) = 0;

	/**
	 * Stop a stand alone file.
	 * @param pIObject - object to stop file on. The audio system guarantees that this is never a null pointer.
	 * @return ERequestStatus - indicates the outcome of underlying process
	 * @see PlayFile
	 */
	virtual ERequestStatus Stop(IObject* const pIObject) = 0;
};
} // namespace Impl
} // namespace CryAudio
