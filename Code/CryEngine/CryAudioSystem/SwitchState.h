// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include "Common/PoolObject.h"

namespace CryAudio
{
class CObject;

class CSwitchState final : public CPoolObject<CSwitchState, stl::PSyncNone>
{
public:

	CSwitchState() = delete;
	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState& operator=(CSwitchState const&) = delete;
	CSwitchState& operator=(CSwitchState&&) = delete;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	explicit CSwitchState(
		ControlId const switchId,
		SwitchStateId const switchStateId,
		SwitchStateConnections const& connections,
		char const* const szName)
		: m_switchStateId(switchStateId)
		, m_switchId(switchId)
		, m_connections(connections)
		, m_name(szName)
	{}
#else
	explicit CSwitchState(
		ControlId const switchId,
		SwitchStateId const switchStateId,
		SwitchStateConnections const& connections)
		: m_switchStateId(switchStateId)
		, m_switchId(switchId)
		, m_connections(connections)
	{}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	~CSwitchState();

	void          Set(CObject const& object) const;
	void          SetGlobally() const;
	SwitchStateId GetId() const { return m_switchStateId; }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

private:

	ControlId const              m_switchId;
	SwitchStateId const          m_switchStateId;
	SwitchStateConnections const m_connections;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
