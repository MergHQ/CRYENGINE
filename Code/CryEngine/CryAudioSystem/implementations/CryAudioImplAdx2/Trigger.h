// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GlobalData.h"
#include <ITrigger.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
enum class ETriggerType : EnumFlagsType
{
	None,
	Trigger,
	Snapshot,
};

enum class EEventType : EnumFlagsType
{
	None,
	Start,
	Stop,
	Pause,
	Resume,
};

class CTrigger final : public ITrigger, public CPoolObject<CTrigger, stl::PSyncNone>
{
public:

	CTrigger() = delete;
	CTrigger(CTrigger const&) = delete;
	CTrigger(CTrigger&&) = delete;
	CTrigger& operator=(CTrigger const&) = delete;
	CTrigger& operator=(CTrigger&&) = delete;

	explicit CTrigger(
		uint32 const id,
		char const* const szCueName,
		uint32 const acbId,
		ETriggerType const triggerType,
		EEventType const eventType,
		CriSint32 const changeoverTime = static_cast<CriSint32>(s_defaultChangeoverTime))
		: m_id(id)
		, m_cueName(szCueName)
		, m_cueSheetId(acbId)
		, m_triggerType(triggerType)
		, m_eventType(eventType)
		, m_changeoverTime(changeoverTime)
	{}

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	explicit CTrigger(
		uint32 const id,
		char const* const szCueName,
		uint32 const acbId,
		ETriggerType const triggerType,
		EEventType const eventType,
		char const* const szCueSheetName,
		CriSint32 const changeoverTime = static_cast<CriSint32>(s_defaultChangeoverTime))
		: m_id(id)
		, m_cueName(szCueName)
		, m_cueSheetId(acbId)
		, m_triggerType(triggerType)
		, m_eventType(eventType)
		, m_cueSheetName(szCueSheetName)
		, m_changeoverTime(changeoverTime)
	{}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITrigger
	virtual ERequestStatus Load()  const override                            { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                           { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITrigger

	uint32          GetId() const             { return m_id; }
	CriChar8 const* GetCueName() const        { return static_cast<CriChar8 const*>(m_cueName); }
	ETriggerType    GetTriggerType() const    { return m_triggerType; }
	EEventType      GetEventType() const      { return m_eventType; }
	uint32          GetAcbId() const          { return m_cueSheetId; }
	CriSint32       GetChangeoverTime() const { return m_changeoverTime; }

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	char const* GetCueSheetName() const { return m_cueSheetName.c_str(); }
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

private:

	uint32 const                                m_id;
	ETriggerType const                          m_triggerType;
	EEventType const                            m_eventType;
	CryFixedStringT<MaxControlNameLength> const m_cueName;
	uint32 const                                m_cueSheetId;
	CriSint32 const                             m_changeoverTime;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_cueSheetName;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
