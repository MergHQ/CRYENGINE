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
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
class CBaseObject;
class CCue;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

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

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	explicit CCueInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const cueId,
		CriAtomExPlaybackId const playbackId,
		CBaseObject const* const pBaseObject,
		CCue const* const pCue)
		: m_triggerInstanceId(triggerInstanceId)
		, m_cueId(cueId)
		, m_playbackId(playbackId)
		, m_flags(ECueInstanceFlags::None)
		, m_pBaseObject(pBaseObject)
		, m_pCue(pCue)
	{}
#else
	explicit CCueInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const cueId,
		CriAtomExPlaybackId const playbackId)
		: m_triggerInstanceId(triggerInstanceId)
		, m_cueId(cueId)
		, m_playbackId(playbackId)
		, m_flags(ECueInstanceFlags::None)
	{}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	~CCueInstance() = default;

	TriggerInstanceId GetTriggerInstanceId() const          { return m_triggerInstanceId; }

	uint32            GetCueId() const                      { return m_cueId; }

	ECueInstanceFlags GetFlags() const                      { return m_flags; }
	void              SetFlag(ECueInstanceFlags const flag) { m_flags |= flag; }

	void              Stop();
	void              Pause();
	void              Resume();

	void              UpdateVirtualState();
	void              UpdatePlaybackState();

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	CBaseObject const* GetObject() const { return m_pBaseObject; }
	CCue const*        GetCue() const    { return m_pCue; }
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

private:

	TriggerInstanceId const   m_triggerInstanceId;
	uint32 const              m_cueId;
	CriAtomExPlaybackId const m_playbackId;
	ECueInstanceFlags         m_flags;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	CBaseObject const* const m_pBaseObject;
	CCue const* const        m_pCue;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
