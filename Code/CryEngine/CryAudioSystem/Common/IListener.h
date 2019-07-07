// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding CryAudio::IListener object (e.g. a middleware-specific unique ID)
 */
struct IListener
{
	/** @cond */
	virtual ~IListener() = default;
	/** @endcond */

	/**
	 * Performs actions that need to be executed regularly on a listener. Called with every tick of the audio thread.
	   virtual void Update(float const deltaTime) = 0;
	 * @param deltaTime - accumulated game frame time in seconds.
	 * @return void
	 */
	virtual void Update(float const deltaTime) = 0;

	/**
	 * Sets the listener's name.
	 * @param szName - name of the listener.
	 * @return void
	 */
	virtual void SetName(char const* const szName) = 0;

	/**
	 * Sets the listener's transformation.
	 * @param transformation - a class containing the listener's position and rotation
	 * @return void
	 */
	virtual void SetTransformation(CTransformation const& transformation) = 0;

	/**
	 * Gets the listener's transformation.
	 * @return CTransformation of the listener.
	 */
	virtual CTransformation const& GetTransformation() const = 0;
};
} // namespace Impl
} // namespace CryAudio
