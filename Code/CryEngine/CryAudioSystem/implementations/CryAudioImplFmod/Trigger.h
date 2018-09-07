// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
enum class EEventType : EnumFlagsType
{
	None,
	Start,
	Stop,
	Pause,
	Resume,
};

class CTrigger final : public ITrigger
{
public:

	CTrigger() = delete;
	CTrigger(CTrigger const&) = delete;
	CTrigger(CTrigger&&) = delete;
	CTrigger& operator=(CTrigger const&) = delete;
	CTrigger& operator=(CTrigger&&) = delete;

	explicit CTrigger(
		uint32 const id,
		EEventType const eventType,
		FMOD::Studio::EventDescription* const pEventDescription,
		FMOD_GUID const guid,
		bool const hasProgrammerSound = false,
		char const* const szKey = "");

	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITrigger
	virtual ERequestStatus Load()  const override                            { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                           { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITrigger

	uint32                                       GetId() const               { return m_id; }
	EEventType                                   GetEventType() const        { return m_eventType; }
	FMOD::Studio::EventDescription*              GetEventDescription() const { return m_pEventDescription; }
	FMOD_GUID                                    GetGuid() const             { return m_guid; }
	bool                                         HasProgrammerSound() const  { return m_hasProgrammerSound; }
	CryFixedStringT<MaxControlNameLength> const& GetKey() const              { return m_key; }

private:

	uint32 const                                m_id;
	EEventType const                            m_eventType;
	FMOD::Studio::EventDescription* const       m_pEventDescription;
	FMOD_GUID const                             m_guid;
	bool const                                  m_hasProgrammerSound;
	CryFixedStringT<MaxControlNameLength> const m_key;
};

using ParameterIdToIndex = std::map<uint32, int>;
using TriggerToParameterIndexes = std::map<CTrigger const* const, ParameterIdToIndex>;

extern TriggerToParameterIndexes g_triggerToParameterIndexes;
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
