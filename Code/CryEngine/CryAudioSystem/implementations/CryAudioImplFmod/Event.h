// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseTriggerConnection.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CEvent final : public CBaseTriggerConnection, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	enum class EActionType : EnumFlagsType
	{
		None,
		Start,
		Stop,
		Pause,
		Resume,
	};

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	// For pure events.
	explicit CEvent(
		uint32 const id,
		EActionType const actionType,
		FMOD_GUID const guid,
		char const* const szName)
		: CBaseTriggerConnection(EType::Event, szName)
		, m_id(id)
		, m_actionType(actionType)
		, m_guid(guid)
		, m_hasProgrammerSound(false)
		, m_numInstances(0)
		, m_toBeDestructed(false)
		, m_key("")
		, m_pEventDescription(nullptr)
	{}

	// For keys/programmer sounds.
	explicit CEvent(
		uint32 const id,
		FMOD_GUID const guid,
		char const* const szKey,
		char const* const szName)
		: CBaseTriggerConnection(EType::Event, szName)
		, m_id(id)
		, m_actionType(EActionType::Start)
		, m_guid(guid)
		, m_hasProgrammerSound(true)
		, m_numInstances(0)
		, m_toBeDestructed(false)
		, m_key(szKey)
		, m_pEventDescription(nullptr)
	{}
#else
	// For pure events.
	explicit CEvent(
		uint32 const id,
		EActionType const actionType,
		FMOD_GUID const guid)
		: CBaseTriggerConnection(EType::Event)
		, m_id(id)
		, m_actionType(actionType)
		, m_guid(guid)
		, m_hasProgrammerSound(false)
		, m_numInstances(0)
		, m_toBeDestructed(false)
		, m_key("")
		, m_pEventDescription(nullptr)
	{}

	// For keys/programmer sounds.
	explicit CEvent(
		uint32 const id,
		FMOD_GUID const guid,
		char const* const szKey)
		: CBaseTriggerConnection(EType::Event)
		, m_id(id)
		, m_actionType(EActionType::Start)
		, m_guid(guid)
		, m_hasProgrammerSound(true)
		, m_numInstances(0)
		, m_toBeDestructed(false)
		, m_key(szKey)
		, m_pEventDescription(nullptr)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	virtual ~CEvent() override;

	// CryAudio::Impl::ITriggerConnection
	virtual ETriggerResult Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	// ~CryAudio::Impl::ITriggerConnection

	uint32                                       GetId() const           { return m_id; }
	FMOD_GUID                                    GetGuid() const         { return m_guid; }
	CryFixedStringT<MaxControlNameLength> const& GetKey() const          { return m_key; }

	void                                         IncrementNumInstances() { ++m_numInstances; }
	void                                         DecrementNumInstances();

	bool                                         CanBeDestructed() const   { return m_toBeDestructed && (m_numInstances == 0); }
	void                                         SetToBeDestructed() const { m_toBeDestructed = true; }

private:

	uint32 const                                m_id;
	EActionType const                           m_actionType;
	FMOD_GUID const                             m_guid;
	bool const                                  m_hasProgrammerSound;
	uint16                                      m_numInstances;
	mutable bool                                m_toBeDestructed;
	CryFixedStringT<MaxControlNameLength> const m_key;
	FMOD::Studio::EventDescription*             m_pEventDescription;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
