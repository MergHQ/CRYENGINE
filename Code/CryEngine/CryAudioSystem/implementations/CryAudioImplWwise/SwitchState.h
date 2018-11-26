// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GlobalData.h"
#include <ISwitchStateConnection.h>
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

class CSwitchState final : public ISwitchStateConnection, public CPoolObject<CSwitchState, stl::PSyncNone>
{
public:

	CSwitchState() = delete;
	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState& operator=(CSwitchState const&) = delete;
	CSwitchState& operator=(CSwitchState&&) = delete;

	explicit CSwitchState(
		ESwitchType const type,
		AkUInt32 const stateOrSwitchGroupId,
		AkUInt32 const stateOrSwitchId,
		float const rtpcValue = s_defaultStateValue)
		: m_type(type)
		, m_stateOrSwitchGroupId(stateOrSwitchGroupId)
		, m_stateOrSwitchId(stateOrSwitchId)
		, m_rtpcValue(rtpcValue)
	{}

	virtual ~CSwitchState() override = default;

	// ISwitchStateConnection
	virtual void Set(IObject* const pIObject) override;
	// ~ISwitchStateConnection

	ESwitchType GetType() const                 { return m_type; }
	AkUInt32    GetStateOrSwitchGroupId() const { return m_stateOrSwitchGroupId; }
	AkUInt32    GetStateOrSwitchId() const      { return m_stateOrSwitchId; }
	float       GetRtpcValue() const            { return m_rtpcValue; }

private:

	ESwitchType const m_type;
	AkUInt32 const    m_stateOrSwitchGroupId;
	AkUInt32 const    m_stateOrSwitchId;
	float const       m_rtpcValue;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
