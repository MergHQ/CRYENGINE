// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchState.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
CSwitchState::CSwitchState(
	ESwitchType const type_,
	AkUInt32 const stateOrSwitchGroupId_,
	AkUInt32 const stateOrSwitchId_,
	float const rtpcValue_ /*= s_defaultStateValue*/)
	: type(type_)
	, stateOrSwitchGroupId(stateOrSwitchGroupId_)
	, stateOrSwitchId(stateOrSwitchId_)
	, rtpcValue(rtpcValue_)
{
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
