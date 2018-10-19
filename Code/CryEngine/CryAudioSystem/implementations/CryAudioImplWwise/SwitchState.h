// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GlobalData.h"
#include <ATLEntityData.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
enum class ESwitchType : EnumFlagsType
{
	None,
	Rtpc,
	StateGroup,
	SwitchGroup,
};

class CSwitchState final : public ISwitchState, public CPoolObject<CSwitchState, stl::PSyncNone>
{
public:

	CSwitchState() = delete;
	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState& operator=(CSwitchState const&) = delete;
	CSwitchState& operator=(CSwitchState&&) = delete;

	explicit CSwitchState(
		ESwitchType const type_,
		AkUInt32 const stateOrSwitchGroupId_,
		AkUInt32 const stateOrSwitchId_,
		float const rtpcValue_ = s_defaultStateValue);

	virtual ~CSwitchState() override = default;

	ESwitchType const type;
	AkUInt32 const    stateOrSwitchGroupId;
	AkUInt32 const    stateOrSwitchId;
	float const       rtpcValue;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
