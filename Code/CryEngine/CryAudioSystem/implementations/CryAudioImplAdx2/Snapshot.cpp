// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Snapshot.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "Common.h"
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
CryFixedStringT<MaxControlNameLength> g_debugActiveSnapShotName = g_debugNoneSnapshot;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
ERequestStatus CSnapshot::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	switch (m_actionType)
	{
	case EActionType::Start:
		{
			criAtomEx_ApplyDspBusSnapshot(static_cast<CriChar8 const*>(m_name), m_changeoverTime);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
			g_debugActiveSnapShotName = m_name;
#endif      // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

			break;
		}
	case EActionType::Stop:
		{
			Stop(nullptr);

			break;
		}
	default:
		{
			break;
		}
	}

	return ERequestStatus::SuccessDoNotTrack;
}

//////////////////////////////////////////////////////////////////////////
void CSnapshot::Stop(IObject* const pIObject)
{
	criAtomEx_ApplyDspBusSnapshot(nullptr, m_changeoverTime);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	g_debugActiveSnapShotName = g_debugNoneSnapshot;
#endif        // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
