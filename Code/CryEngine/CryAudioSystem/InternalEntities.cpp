// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "InternalEntities.h"
#include "AudioListenerManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include "Common.h"
#include <IAudioImpl.h>

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
COcclusionObstructionState::COcclusionObstructionState(SwitchStateId const stateId)
	: m_stateId(stateId)
{
}

//////////////////////////////////////////////////////////////////////////
void COcclusionObstructionState::Set(CATLAudioObject& audioObject) const
{
	if (&audioObject != g_pObject)
	{
		if (m_stateId == IgnoreStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Ignore);
			audioObject.SetObstructionOcclusion(0.0f, 0.0f);
		}
		else if (m_stateId == AdaptiveStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Adaptive);
		}
		else if (m_stateId == LowStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Low);
		}
		else if (m_stateId == MediumStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Medium);
		}
		else if (m_stateId == HighStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::High);
		}
		else
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Ignore);
			audioObject.SetObstructionOcclusion(0.0f, 0.0f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CRelativeVelocityTrackingState::CRelativeVelocityTrackingState(SwitchStateId const stateId)
	: m_stateId(stateId)
{
}

//////////////////////////////////////////////////////////////////////////
void CRelativeVelocityTrackingState::Set(CATLAudioObject& audioObject) const
{
	if (&audioObject != g_pObject)
	{
		if (m_stateId == OnStateId)
		{
			audioObject.SetFlag(EObjectFlags::TrackRelativeVelocity);
		}
		else if (m_stateId == OffStateId)
		{
			audioObject.RemoveFlag(EObjectFlags::TrackRelativeVelocity);
		}
		else
		{
			CRY_ASSERT(false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CAbsoluteVelocityTrackingState::CAbsoluteVelocityTrackingState(SwitchStateId const stateId)
	: m_stateId(stateId)
{
}

//////////////////////////////////////////////////////////////////////////
void CAbsoluteVelocityTrackingState::Set(CATLAudioObject& audioObject) const
{
	if (&audioObject != g_pObject)
	{
		if (m_stateId == OnStateId)
		{
			audioObject.SetFlag(EObjectFlags::TrackAbsoluteVelocity);
		}
		else if (m_stateId == OffStateId)
		{
			audioObject.RemoveFlag(EObjectFlags::TrackAbsoluteVelocity);
		}
		else
		{
			CRY_ASSERT(false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CDoNothingTrigger::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	return ERequestStatus::SuccessDoNotTrack;
}
} // namespace CryAudio
