// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GlobalData.h"
#include <ITriggerConnection.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CCue final : public ITriggerConnection, public CPoolObject<CCue, stl::PSyncNone>
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

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	explicit CCue(
		uint32 const id,
		char const* const szCueName,
		uint32 const acbId,
		EActionType const type,
		char const* const szCueSheetName)
		: m_id(id)
		, m_name(szCueName)
		, m_cueSheetId(acbId)
		, m_actionType(type)
		, m_cueSheetName(szCueSheetName)
	{}
#else
	explicit CCue(
		uint32 const id,
		char const* const szCueName,
		uint32 const acbId,
		EActionType const type)
		: m_id(id)
		, m_name(szCueName)
		, m_cueSheetId(acbId)
		, m_actionType(type)
	{}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	virtual ~CCue() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ETriggerResult Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	virtual ERequestStatus Load()  const override                                                { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                               { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(TriggerInstanceId const triggerInstanceId) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(TriggerInstanceId const triggerInstanceId) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITriggerConnection

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

private:

	uint32 const                                m_id;
	EActionType const                           m_actionType;
	CryFixedStringT<MaxControlNameLength> const m_name;
	uint32 const                                m_cueSheetId;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_cueSheetName;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
