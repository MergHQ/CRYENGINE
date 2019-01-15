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
enum class EActionType : EnumFlagsType
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
		EActionType const actionType,
		FMOD::Studio::EventDescription* const pEventDescription,
		FMOD_GUID const guid,
		bool const hasProgrammerSound = false,
		char const* const szKey = "")
		: m_id(id)
		, m_actionType(actionType)
		, m_pEventDescription(pEventDescription)
		, m_guid(guid)
		, m_hasProgrammerSound(hasProgrammerSound)
		, m_key(szKey)
	{}

	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ERequestStatus Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	virtual ERequestStatus Load()  const override                                                { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                               { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(TriggerInstanceId const triggerInstanceId) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(TriggerInstanceId const triggerInstanceId) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITriggerConnection

	uint32                                       GetId() const   { return m_id; }
	FMOD_GUID                                    GetGuid() const { return m_guid; }
	CryFixedStringT<MaxControlNameLength> const& GetKey() const  { return m_key; }

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	void SetName(char const* const szName) { m_name = szName; }
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

private:

	uint32 const                                m_id;
	EActionType const                           m_actionType;
	FMOD::Studio::EventDescription*             m_pEventDescription;
	FMOD_GUID const                             m_guid;
	bool const                                  m_hasProgrammerSound;
	CryFixedStringT<MaxControlNameLength> const m_key;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
