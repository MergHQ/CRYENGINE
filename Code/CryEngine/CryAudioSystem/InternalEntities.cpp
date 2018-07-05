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
COcclusionObstructionState::COcclusionObstructionState(SwitchStateId const stateId, CAudioListenerManager const& audioListenerManager)
	: m_stateId(stateId)
	, m_audioListenerManager(audioListenerManager)
{
}

//////////////////////////////////////////////////////////////////////////
void COcclusionObstructionState::Set(CATLAudioObject& audioObject) const
{
	if (&audioObject != g_pObject)
	{
		Vec3 const& audioListenerPosition = m_audioListenerManager.GetActiveListenerTransformation().GetPosition();

		if (m_stateId == IgnoreStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Ignore, audioListenerPosition);
			audioObject.SetObstructionOcclusion(0.0f, 0.0f);
		}
		else if (m_stateId == AdaptiveStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Adaptive, audioListenerPosition);
		}
		else if (m_stateId == LowStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Low, audioListenerPosition);
		}
		else if (m_stateId == MediumStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Medium, audioListenerPosition);
		}
		else if (m_stateId == HighStateId)
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::High, audioListenerPosition);
		}
		else
		{
			audioObject.HandleSetOcclusionType(EOcclusionType::Ignore, audioListenerPosition);
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

//////////////////////////////////////////////////////////////////////////
ERequestStatus CLoseFocusTrigger::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	ERequestStatus const result = g_pIImpl->OnLoseFocus();
	return result == ERequestStatus::Success ? ERequestStatus::SuccessDoNotTrack : result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CGetFocusTrigger::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	ERequestStatus const result = g_pIImpl->OnGetFocus();
	return result == ERequestStatus::Success ? ERequestStatus::SuccessDoNotTrack : result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CMuteAllTrigger::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	ERequestStatus const result = g_pIImpl->MuteAll();
	return result == ERequestStatus::Success ? ERequestStatus::SuccessDoNotTrack : result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CUnmuteAllTrigger::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	ERequestStatus const result = g_pIImpl->UnmuteAll();
	return result == ERequestStatus::Success ? ERequestStatus::SuccessDoNotTrack : result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CPauseAllTrigger::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	ERequestStatus const result = g_pIImpl->PauseAll();
	return result == ERequestStatus::Success ? ERequestStatus::SuccessDoNotTrack : result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CResumeAllTrigger::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	ERequestStatus const result = g_pIImpl->ResumeAll();
	return result == ERequestStatus::Success ? ERequestStatus::SuccessDoNotTrack : result;
}

//////////////////////////////////////////////////////////////////////////
void CAbsoluteVelocityParameter::Set(CATLAudioObject& audioObject, float const value) const
{
}

//////////////////////////////////////////////////////////////////////////
void CRelativeVelocityParameter::Set(CATLAudioObject& audioObject, float const value) const
{
}
} // namespace CryAudio
