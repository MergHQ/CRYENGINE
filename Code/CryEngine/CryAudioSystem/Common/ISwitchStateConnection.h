// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
struct IObject;

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding ISwitchState
 * (e.g. a middleware-specific control ID or a switch and state names to be passed to an API function)
 */
struct ISwitchStateConnection
{
	/** @cond */
	virtual ~ISwitchStateConnection() = default;
	/** @endcond */

	/**
	 * Set the state (on a switch) on the provided object.
	 * @param pIObject - implementation-specific object to set the state on. The audio system guarantees that this is never a null pointer.
	 * @return void
	 */
	virtual void Set(IObject* const pIObject) = 0;

	/**
	 * Set the state (on a switch) globally.
	 * @return void
	 */
	virtual void SetGlobally() = 0;
};
} // namespace Impl
} // namespace CryAudio
