// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
class CATLEvent;

namespace Impl
{
namespace Adx2
{
class CBaseObject;

enum class EEventFlags : EnumFlagsType
{
	None                = 0,
	HasAbsoluteVelocity = BIT(0),
	HasDoppler          = BIT(1),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EEventFlags);

class CEvent final : public IEvent, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(CATLEvent& event);
	virtual ~CEvent() override;

	// CryAudio::Impl::IEvent
	virtual ERequestStatus Stop() override;
	// ~CryAudio::Impl::IEvent

	CATLEvent&          GetATLEvent() const                                 { return m_event; }
	void                SetObject(CBaseObject* const pObject)               { m_pObject = pObject; }

	uint32              GetTriggerId() const                                { return m_triggerId; }
	void                SetTriggerId(uint32 const triggerId)                { m_triggerId = triggerId; }

	CriAtomExPlaybackId GetPlaybackId() const                               { return m_playbackId; }
	void                SetPlaybackId(CriAtomExPlaybackId const playbackId) { m_playbackId = playbackId; }

	EEventFlags         GetFlags() const                                    { return m_flags; }
	void                SetFlag(EEventFlags const flag)                     { m_flags |= flag; }

	void                Pause();
	void                Resume();

private:

	CATLEvent&          m_event;
	CBaseObject*        m_pObject;
	uint32              m_triggerId;
	CriAtomExPlaybackId m_playbackId;
	EEventFlags         m_flags;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
