// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IFile.
 * (e.g. a middleware-specific bank ID if the AudioFileEntry represents a soundbank)
 */
struct IFile
{
	/** @cond */
	virtual ~IFile() = default;
	/** @endcond */
};
} // namespace Impl
} // namespace CryAudio
