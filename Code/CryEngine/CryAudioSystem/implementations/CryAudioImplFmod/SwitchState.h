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

class CSwitchState : public ISwitchState
{
public:

	CSwitchState() = delete;
	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState& operator=(CSwitchState const&) = delete;
	CSwitchState& operator=(CSwitchState&&) = delete;

	explicit CSwitchState(
		uint32 const id,
		float const value,
		char const* const szName,
		EStateType const type);

	virtual ~CSwitchState() override = default;

	uint32                                       GetId() const    { return m_id; }
	float                                        GetValue() const { return m_value; }
	EStateType                                   GetType() const  { return m_type; }
	CryFixedStringT<MaxControlNameLength> const& GetName() const  { return m_name; }

private:

	uint32 const                                m_id;
	float const                                 m_value;
	EStateType const                            m_type;
	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
