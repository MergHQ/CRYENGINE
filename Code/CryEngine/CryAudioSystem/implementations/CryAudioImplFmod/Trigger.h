// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ITriggerConnection.h>
#include <PoolObject.h>

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

class CTrigger final : public ITriggerConnection, public CPoolObject<CTrigger, stl::PSyncNone>
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
		char const* const szKey = "")
		: m_id(id)
		, m_eventType(eventType)
		, m_pEventDescription(pEventDescription)
		, m_guid(guid)
		, m_hasProgrammerSound(hasProgrammerSound)
		, m_key(szKey)
	{}

	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ERequestStatus Execute(IObject* const pIObject, IEvent* const pIEvent) override;
	virtual ERequestStatus Load()  const override                            { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                           { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITriggerConnection

	uint32                                       GetId() const   { return m_id; }
	FMOD_GUID                                    GetGuid() const { return m_guid; }
	CryFixedStringT<MaxControlNameLength> const& GetKey() const  { return m_key; }

private:

	uint32 const                                m_id;
	EEventType const                            m_eventType;
	FMOD::Studio::EventDescription*             m_pEventDescription;
	FMOD_GUID const                             m_guid;
	bool const                                  m_hasProgrammerSound;
	CryFixedStringT<MaxControlNameLength> const m_key;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
