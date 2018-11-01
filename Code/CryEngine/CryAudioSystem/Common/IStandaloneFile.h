// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IStandaloneFile.
 * (e.g. middleware-specific custom data that is associated with the standalone file)
 */
struct IStandaloneFile
{
	/** @cond */
	virtual ~IStandaloneFile() = default;
	/** @endcond */
};
} // namespace Impl
} // namespace CryAudio
