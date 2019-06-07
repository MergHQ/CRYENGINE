// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseTriggerConnection.h"
#include "GlobalData.h"
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CCue final : public CBaseTriggerConnection, public CPoolObject<CCue, stl::PSyncNone>
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

	CCue() = delete;
	CCue(CCue const&) = delete;
	CCue(CCue&&) = delete;
	CCue& operator=(CCue const&) = delete;
	CCue& operator=(CCue&&) = delete;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	explicit CCue(
		uint32 const id,
		char const* const szCueName,
		uint32 const acbId,
		EActionType const type,
		char const* const szCueSheetName,
		float const fadeOutTime)
		: CBaseTriggerConnection(EType::Cue, szCueName)
		, m_id(id)
		, m_cueSheetId(acbId)
		, m_actionType(type)
		, m_pAcbHandle(nullptr)
		, m_cueSheetName(szCueSheetName)
		, m_fadeOutTime(fadeOutTime)
		, m_numInstances(0)
		, m_toBeDestructed(false)
	{}
#else
	explicit CCue(
		uint32 const id,
		char const* const szCueName,
		uint32 const acbId,
		EActionType const type)
		: CBaseTriggerConnection(EType::Cue, szCueName)
		, m_id(id)
		, m_cueSheetId(acbId)
		, m_actionType(type)
		, m_pAcbHandle(nullptr)
		, m_numInstances(0)
		, m_toBeDestructed(false)
	{}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE

	virtual ~CCue() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ETriggerResult Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	// ~CryAudio::Impl::ITriggerConnection

	uint32         GetId() const           { return m_id; }
	CriAtomExAcbHn GetAcbHandle() const    { return m_pAcbHandle; }

	void           IncrementNumInstances() { ++m_numInstances; }
	void           DecrementNumInstances();

	bool           CanBeDestructed() const { return m_toBeDestructed && (m_numInstances == 0); }
	void           SetToBeDestructed()     { m_toBeDestructed = true; }

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	float GetFadeOutTime() const { return m_fadeOutTime; }
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE

private:

	uint32 const      m_id;
	uint32 const      m_cueSheetId;
	EActionType const m_actionType;
	CriAtomExAcbHn    m_pAcbHandle;
	uint16            m_numInstances;
	bool              m_toBeDestructed;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_cueSheetName;
	float const m_fadeOutTime;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
