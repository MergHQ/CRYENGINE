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
class CEvent final : public ITriggerConnection, public CPoolObject<CEvent, stl::PSyncNone>
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
		: m_id(id)
		, m_actionType(actionType)
		, m_guid(guid)
		, m_hasProgrammerSound(false)
		, m_key("")
		, m_pEventDescription(nullptr)
		, m_name(szName)
	{}

	// For keys/programmer sounds.
	explicit CEvent(
		uint32 const id,
		FMOD_GUID const guid,
		char const* const szKey,
		char const* const szName)
		: m_id(id)
		, m_actionType(EActionType::Start)
		, m_guid(guid)
		, m_hasProgrammerSound(true)
		, m_key(szKey)
		, m_pEventDescription(nullptr)
		, m_name(szName)
	{}
#else
	// For pure events.
	explicit CEvent(
		uint32 const id,
		EActionType const actionType,
		FMOD_GUID const guid)
		: m_id(id)
		, m_actionType(actionType)
		, m_guid(guid)
		, m_hasProgrammerSound(false)
		, m_key("")
		, m_pEventDescription(nullptr)
	{}

	// For keys/programmer sounds.
	explicit CEvent(
		uint32 const id,
		FMOD_GUID const guid,
		char const* const szKey)
		: m_id(id)
		, m_actionType(EActionType::Start)
		, m_guid(guid)
		, m_hasProgrammerSound(true)
		, m_key(szKey)
		, m_pEventDescription(nullptr)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	virtual ~CEvent() override;

	// CryAudio::Impl::ITriggerConnection
	virtual ETriggerResult Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	virtual ERequestStatus Load()  const override                                                { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                               { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(TriggerInstanceId const triggerInstanceId) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(TriggerInstanceId const triggerInstanceId) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITriggerConnection

	uint32                                       GetId() const   { return m_id; }
	FMOD_GUID                                    GetGuid() const { return m_guid; }
	CryFixedStringT<MaxControlNameLength> const& GetKey() const  { return m_key; }

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

private:

	uint32 const                                m_id;
	EActionType const                           m_actionType;
	FMOD_GUID const                             m_guid;
	bool const                                  m_hasProgrammerSound;
	CryFixedStringT<MaxControlNameLength> const m_key;
	FMOD::Studio::EventDescription*             m_pEventDescription;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
