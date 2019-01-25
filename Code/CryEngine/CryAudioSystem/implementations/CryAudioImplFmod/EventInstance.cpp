// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EventInstance.h"
#include "BaseObject.h"
#include "CVars.h"
#include "Return.h"
#include "Event.h"
#include <CryAudio/IAudioSystem.h>

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CEventInstance::~CEventInstance()
{
	if (m_pInstance != nullptr)
	{
		FMOD_RESULT const fmodResult = m_pInstance->release();
		ASSERT_FMOD_OK_OR_INVALID_HANDLE;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::SetInternalParameters()
{
	m_pInstance->getParameter(g_szOcclusionParameterName, &m_pOcclusionParameter);
	m_pInstance->getParameter(g_szAbsoluteVelocityParameterName, &m_pAbsoluteVelocityParameter);
}

//////////////////////////////////////////////////////////////////////////
bool CEventInstance::PrepareForOcclusion()
{
	m_pMasterTrack = nullptr;
	FMOD_RESULT fmodResult = m_pInstance->getChannelGroup(&m_pMasterTrack);
	ASSERT_FMOD_OK_OR_NOT_LOADED;

	if ((m_pMasterTrack != nullptr) && (m_pOcclusionParameter == nullptr))
	{
		m_pLowpass = nullptr;
		int numDSPs = 0;
		fmodResult = m_pMasterTrack->getNumDSPs(&numDSPs);
		ASSERT_FMOD_OK;

		for (int i = 0; i < numDSPs; ++i)
		{
			fmodResult = m_pMasterTrack->getDSP(i, &m_pLowpass);
			ASSERT_FMOD_OK;

			if (m_pLowpass != nullptr)
			{
				FMOD_DSP_TYPE dspType;
				fmodResult = m_pLowpass->getType(&dspType);
				ASSERT_FMOD_OK;

				if (dspType == FMOD_DSP_TYPE_LOWPASS_SIMPLE || dspType == FMOD_DSP_TYPE_LOWPASS)
				{
					FMOD_DSP_PARAMETER_DESC* pParameterDesc = nullptr;
					fmodResult = m_pLowpass->getParameterInfo(FMOD_DSP_LOWPASS_CUTOFF, &pParameterDesc);
					ASSERT_FMOD_OK;

					m_lowpassFrequencyMin = pParameterDesc->floatdesc.min;
					m_lowpassFrequencyMax = pParameterDesc->floatdesc.max;
					break;
				}
				else
				{
					m_pLowpass = nullptr;
				}
			}
		}
	}

	return m_pMasterTrack != nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::SetOcclusion(float const occlusion)
{
	if (m_pOcclusionParameter != nullptr)
	{
		FMOD_RESULT const fmodResult = m_pOcclusionParameter->setValue(occlusion);
		ASSERT_FMOD_OK;
	}
	else if (m_pLowpass != nullptr)
	{
		float const range = m_lowpassFrequencyMax - std::max(m_lowpassFrequencyMin, g_cvars.m_lowpassMinCutoffFrequency);
		float const value = m_lowpassFrequencyMax - (occlusion * range);
		FMOD_RESULT const fmodResult = m_pLowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, value);
		ASSERT_FMOD_OK;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::SetReturnSend(CReturn const* const pReturn, float const value)
{
	if ((m_pInstance != nullptr) && (m_pMasterTrack != nullptr))
	{
		FMOD::ChannelGroup* pChannelGroup = nullptr;
		FMOD_RESULT fmodResult = pReturn->GetBus()->getChannelGroup(&pChannelGroup);
		ASSERT_FMOD_OK;

		if (pChannelGroup != nullptr)
		{
			FMOD::DSP* pDsp = nullptr;
			fmodResult = pChannelGroup->getDSP(FMOD_CHANNELCONTROL_DSP_TAIL, &pDsp);
			ASSERT_FMOD_OK;

			if (pDsp != nullptr)
			{
				int returnId1 = FMOD_IMPL_INVALID_INDEX;
				fmodResult = pDsp->getParameterInt(FMOD_DSP_RETURN_ID, &returnId1, nullptr, 0);
				ASSERT_FMOD_OK;

				int numDSPs = 0;
				fmodResult = m_pMasterTrack->getNumDSPs(&numDSPs);
				ASSERT_FMOD_OK;

				for (int i = 0; i < numDSPs; ++i)
				{
					FMOD::DSP* pSend = nullptr;
					fmodResult = m_pMasterTrack->getDSP(i, &pSend);
					ASSERT_FMOD_OK;

					if (pSend != nullptr)
					{
						FMOD_DSP_TYPE dspType;
						fmodResult = pSend->getType(&dspType);
						ASSERT_FMOD_OK;

						if (dspType == FMOD_DSP_TYPE_SEND)
						{
							int returnId2 = FMOD_IMPL_INVALID_INDEX;
							fmodResult = pSend->getParameterInt(FMOD_DSP_RETURN_ID, &returnId2, nullptr, 0);
							ASSERT_FMOD_OK;

							if (returnId1 == returnId2)
							{
								fmodResult = pSend->setParameterFloat(FMOD_DSP_SEND_LEVEL, value);
								ASSERT_FMOD_OK;
								break;
							}
						}
					}
				}
			}
		}
	}
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Event instance or master track of %s does not exist during %s", m_pEvent->GetName(), __FUNCTION__);
	}
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::UpdateVirtualState()
{
	// Workaround until Fmod has callbacks for virtual/physical states.
	if (m_pMasterTrack != nullptr)
	{
		float audibility = 0.0f;
		m_pMasterTrack->getAudibility(&audibility);

		m_state = (audibility < 0.01f) ? EEventState::Virtual : EEventState::Playing;
	}
	else
	{
		m_state = EEventState::None;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::SetAbsoluteVelocity(float const velocity)
{
	if (m_pAbsoluteVelocityParameter != nullptr)
	{
		FMOD_RESULT const fmodResult = m_pAbsoluteVelocityParameter->setValue(velocity);
		ASSERT_FMOD_OK;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::StopAllowFadeOut()
{
	FMOD_RESULT const fmodResult = m_pInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
	ASSERT_FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
void CEventInstance::StopImmediate()
{
	FMOD_RESULT const fmodResult = m_pInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
	ASSERT_FMOD_OK;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
