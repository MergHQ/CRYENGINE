// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
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
};
} // namespace Impl
} // namespace CryAudio
