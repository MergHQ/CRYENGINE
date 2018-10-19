// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
enum class EStateType : EnumFlagsType
{
	None,
	State,
	VCA,
};

class CBaseSwitchState : public ISwitchState
{
public:

	CBaseSwitchState() = delete;
	CBaseSwitchState(CBaseSwitchState const&) = delete;
	CBaseSwitchState(CBaseSwitchState&&) = delete;
	CBaseSwitchState& operator=(CBaseSwitchState const&) = delete;
	CBaseSwitchState& operator=(CBaseSwitchState&&) = delete;

	virtual ~CBaseSwitchState() override = default;

	uint32     GetId() const    { return m_id; }
	float      GetValue() const { return m_value; }
	EStateType GetType() const  { return m_type; }

protected:

	explicit CBaseSwitchState(
		uint32 const id,
		float const value,
		EStateType const type);

	uint32 const     m_id;
	float const      m_value;
	EStateType const m_type;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
