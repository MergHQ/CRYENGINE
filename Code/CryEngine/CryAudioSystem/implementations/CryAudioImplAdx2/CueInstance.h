// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <PoolObject.h>
#include <atomic>
#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
class CBaseObject;
class CCue;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

enum class ECueInstanceFlags : EnumFlagsType
{
	None                = 0,
	IsVirtual           = BIT(0),
	HasAbsoluteVelocity = BIT(1),
	HasDoppler          = BIT(2),
	ToBeRemoved         = BIT(3),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ECueInstanceFlags);

class CCueInstance final : public CPoolObject<CCueInstance, stl::PSyncNone>
{
public:

	CCueInstance() = delete;
	CCueInstance(CCueInstance const&) = delete;
	CCueInstance(CCueInstance&&) = delete;
	CCueInstance& operator=(CCueInstance const&) = delete;
	CCueInstance& operator=(CCueInstance&&) = delete;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	explicit CCueInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const cueId,
		CriAtomExPlaybackId const playbackId,
		CCue const* const pCue,
		CBaseObject const* const pBaseObject)
		: m_triggerInstanceId(triggerInstanceId)
		, m_cueId(cueId)
		, m_playbackId(playbackId)
		, m_pCue(pCue)
		, m_flags(ECueInstanceFlags::IsVirtual) // Voices always start in virtual state.
		, m_pBaseObject(pBaseObject)
	{}
#else
	explicit CCueInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const cueId,
		CriAtomExPlaybackId const playbackId,
		CCue const* const pCue)
		: m_triggerInstanceId(triggerInstanceId)
		, m_cueId(cueId)
		, m_playbackId(playbackId)
		, m_pCue(pCue)
		, m_flags(ECueInstanceFlags::None)
	{}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	~CCueInstance() = default;

	TriggerInstanceId   GetTriggerInstanceId() const             { return m_triggerInstanceId; }

	uint32              GetCueId() const                         { return m_cueId; }
	CriAtomExPlaybackId GetPlaybackId() const                    { return m_playbackId; }

	ECueInstanceFlags   GetFlags() const                         { return m_flags; }
	void                SetFlag(ECueInstanceFlags const flag)    { m_flags = m_flags | flag; }
	void                RemoveFlag(ECueInstanceFlags const flag) { m_flags = m_flags & ~flag; }

	bool                PrepareForPlayback();

	void                Stop();
	void                Pause();
	void                Resume();

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	CBaseObject const* GetObject() const { return m_pBaseObject; }
	CCue const*        GetCue() const    { return m_pCue; }
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

private:

	TriggerInstanceId const        m_triggerInstanceId;
	uint32 const                   m_cueId;
	CriAtomExPlaybackId const      m_playbackId;
	std::atomic<ECueInstanceFlags> m_flags;
	CCue const* const              m_pCue;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	CBaseObject const* const m_pBaseObject;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
