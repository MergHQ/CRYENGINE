// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
struct IObject;

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IParameter
 * (e.g. a middleware-specific control ID or a parameter name to be passed to an API function)
 */
struct IParameterConnection
{
	/** @cond */
	virtual ~IParameterConnection() = default;
	/** @endcond */

	/**
	 * Set the parameter to the specified value on the provided object.
	 * @param pIObject - implementation-specific object to set the parameter on. The audio system guarantees that this is never a null pointer.
	 * @param value - the value to set the parameter to
	 * @return void
	 */
	virtual void Set(IObject* const pIObject, float const value) = 0;

	/**
	 * Set the parameter to the specified value globally.
	 * @param value - the value to set the parameter to
	 * @return void
	 */
	virtual void SetGlobally(float const value) = 0;
};
} // namespace Impl
} // namespace CryAudio
