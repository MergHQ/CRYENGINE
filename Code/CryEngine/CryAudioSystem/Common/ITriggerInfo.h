// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
/**
 * @struct ITriggerInfo
 * @brief interface that is used to pass middleware specific information for constructing a trigger without an XML node.
 */
struct ITriggerInfo
{
	/** @cond */
	virtual ~ITriggerInfo() = default;
	/** @endcond */
};
} // namespace Impl
} // namespace CryAudio
