// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <PoolObject.h>
#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CBaseObject;

enum class EEventFlags : EnumFlagsType
{
	None                = 0,
	IsVirtual           = BIT(0),
	HasAbsoluteVelocity = BIT(1),
	HasDoppler          = BIT(2),
	ToBeRemoved         = BIT(3),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EEventFlags);

class CEvent final : public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(TriggerInstanceId const triggerInstanceId)
		: m_triggerInstanceId(triggerInstanceId)
		, m_pObject(nullptr)
		, m_triggerId(InvalidCRC32)
		, m_playbackId(CRIATOMEX_INVALID_PLAYBACK_ID)
		, m_flags(EEventFlags::None)
	{}

	~CEvent() = default;

	TriggerInstanceId   GetTriggerInstanceId() const                        { return m_triggerInstanceId; }

	void                SetObject(CBaseObject* const pObject)               { m_pObject = pObject; }

	uint32              GetTriggerId() const                                { return m_triggerId; }
	void                SetTriggerId(uint32 const triggerId)                { m_triggerId = triggerId; }

	CriAtomExPlaybackId GetPlaybackId() const                               { return m_playbackId; }
	void                SetPlaybackId(CriAtomExPlaybackId const playbackId) { m_playbackId = playbackId; }

	EEventFlags         GetFlags() const                                    { return m_flags; }
	void                SetFlag(EEventFlags const flag)                     { m_flags |= flag; }

	void                Stop();
	void                Pause();
	void                Resume();

	void                UpdateVirtualState();
	void                UpdatePlaybackState();

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	void         SetName(char const* const szName) { m_name = szName; }
	char const*  GetName() const                   { return m_name.c_str(); }
	CBaseObject* GetObject() const                 { return m_pObject; }
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

private:

	TriggerInstanceId const m_triggerInstanceId;
	CBaseObject*            m_pObject;
	uint32                  m_triggerId;
	CriAtomExPlaybackId     m_playbackId;
	EEventFlags             m_flags;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
