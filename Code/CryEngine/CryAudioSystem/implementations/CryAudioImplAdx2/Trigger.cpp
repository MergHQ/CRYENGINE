// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
CTrigger::CTrigger(
	uint32 const id,
	char const* const szCueName,
	uint32 const acbId,
	ETriggerType const triggerType,
	EEventType const eventType,
	CriSint32 const changeoverTime /*= static_cast<CriSint32>(s_defaultChangeoverTime)*/)
	: m_id(id)
	, m_cueName(szCueName)
	, m_cueSheetId(acbId)
	, m_triggerType(triggerType)
	, m_eventType(eventType)
	, m_changeoverTime(changeoverTime)
{
}

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
CTrigger::CTrigger(
	uint32 const id,
	char const* const szCueName,
	uint32 const acbId,
	ETriggerType const triggerType,
	EEventType const eventType,
	char const* const szCueSheetName,
	CriSint32 const changeoverTime /*= static_cast<CriSint32>(s_defaultChangeoverTime)*/)
	: m_id(id)
	, m_cueName(szCueName)
	, m_cueSheetId(acbId)
	, m_triggerType(triggerType)
	, m_eventType(eventType)
	, m_cueSheetName(szCueSheetName)
	, m_changeoverTime(changeoverTime)
{
}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
CryAudio::ERequestStatus CTrigger::Load() const
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
CryAudio::ERequestStatus CTrigger::Unload() const
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
CryAudio::ERequestStatus CTrigger::LoadAsync(IEvent* const pIEvent) const
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
CryAudio::ERequestStatus CTrigger::UnloadAsync(IEvent* const pIEvent) const
{
	return ERequestStatus::Success;
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
