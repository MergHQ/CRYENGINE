// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
struct IObject;

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IEnvironmentConnection
 * (e.g. a middleware-specific bus ID or name to be passed to an API function)
 */
struct IEnvironmentConnection
{
	/** @cond */
	virtual ~IEnvironmentConnection() = default;
	/** @endcond */

	/**
	 * Set the provided value for the environment on the specified object.
	 * @param pIObject - implementation-specific object to set the environment on. The audio system guarantees that this is never a null pointer.
	 * @param amount - the fade value for the environment, 0.0f means no effect at all, 1.0f corresponds to the full effect
	 * @return void
	 */
	virtual void Set(IObject* const pIObject, float const amount) = 0;
};
} // namespace Impl
} // namespace CryAudio
