// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding ISetting
 * (e.g. a middleware-specific setting ID or name to be passed to an API function)
 */
struct ISettingConnection
{
	/** @cond */
	virtual ~ISettingConnection() = default;
	/** @endcond */

	/**
	 * Load the setting
	 * Loading settings manually is only necessary if their data have not been loaded automatically
	 * @return void
	 * @see Unload
	 */
	virtual void Load() = 0;

	/**
	 * Unload the setting
	 * @return void
	 * @see Load
	 */
	virtual void Unload() = 0;
};
} // namespace Impl
} // namespace CryAudio
