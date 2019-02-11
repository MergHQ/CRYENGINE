// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <atomic>

namespace FMOD
{
class ChannelGroup;
class DSP;

namespace Studio
{
class EventInstance;
class ParameterInstance;
} // namespace Studio
} // namespace FMOD

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CBaseObject;
class CEvent;
class CReturn;

enum class EEventState : EnumFlagsType
{
	None,
	Playing,
	Virtual,
	Pending,
};

class CEventInstance final : public CPoolObject<CEventInstance, stl::PSyncNone>
{
public:

	CEventInstance() = delete;
	CEventInstance(CEventInstance const&) = delete;
	CEventInstance(CEventInstance&&) = delete;
	CEventInstance& operator=(CEventInstance const&) = delete;
	CEventInstance& operator=(CEventInstance&&) = delete;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	explicit CEventInstance(TriggerInstanceId const triggerInstanceId, CEvent const& event, CBaseObject const& baseObject)
		: m_triggerInstanceId(triggerInstanceId)
		, m_event(event)
		, m_state(EEventState::Pending)
		, m_lowpassFrequencyMax(0.0f)
		, m_lowpassFrequencyMin(0.0f)
		, m_pInstance(nullptr)
		, m_pMasterTrack(nullptr)
		, m_pLowpass(nullptr)
		, m_pOcclusionParameter(nullptr)
		, m_pAbsoluteVelocityParameter(nullptr)
		, m_toBeRemoved(false)
		, m_isFadingOut(false)
		, m_baseObject(baseObject)
	{}
#else
	explicit CEventInstance(TriggerInstanceId const triggerInstanceId, CEvent const& event)
		: m_triggerInstanceId(triggerInstanceId)
		, m_event(event)
		, m_state(EEventState::Pending)
		, m_lowpassFrequencyMax(0.0f)
		, m_lowpassFrequencyMin(0.0f)
		, m_pInstance(nullptr)
		, m_pMasterTrack(nullptr)
		, m_pLowpass(nullptr)
		, m_pOcclusionParameter(nullptr)
		, m_pAbsoluteVelocityParameter(nullptr)
		, m_toBeRemoved(false)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	~CEventInstance();

	TriggerInstanceId            GetTriggerInstanceId() const                                       { return m_triggerInstanceId; }
	CEvent const&                GetEvent() const                                                   { return m_event; }
	EEventState                  GetState() const                                                   { return m_state; }

	FMOD::Studio::EventInstance* GetFmodEventInstance() const                                       { return m_pInstance; }
	void                         SetFmodEventInstance(FMOD::Studio::EventInstance* const pInstance) { m_pInstance = pInstance; }

	bool                         HasAbsoluteVelocityParameter() const                               { return m_pAbsoluteVelocityParameter != nullptr; }

	void                         SetInternalParameters();
	bool                         PrepareForOcclusion();
	void                         SetOcclusion(float const occlusion);
	void                         SetReturnSend(CReturn const* const pReturn, float const value);
	void                         UpdateVirtualState();
	void                         SetAbsoluteVelocity(float const value);
	void                         StopAllowFadeOut();
	void                         StopImmediate();

	void                         SetToBeRemoved()      { m_toBeRemoved = true; }
	bool                         IsToBeRemoved() const { return m_toBeRemoved; }

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	bool               IsFadingOut() const { return m_isFadingOut; }
	CBaseObject const& GetObject() const   { return m_baseObject; }
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

private:

	TriggerInstanceId const          m_triggerInstanceId;
	CEvent const&                    m_event;

	EEventState                      m_state;

	float                            m_lowpassFrequencyMax;
	float                            m_lowpassFrequencyMin;

	FMOD::Studio::EventInstance*     m_pInstance;
	FMOD::ChannelGroup*              m_pMasterTrack;
	FMOD::DSP*                       m_pLowpass;
	FMOD::Studio::ParameterInstance* m_pOcclusionParameter;
	FMOD::Studio::ParameterInstance* m_pAbsoluteVelocityParameter;

	std::atomic_bool                 m_toBeRemoved;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	bool               m_isFadingOut;
	CBaseObject const& m_baseObject;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
