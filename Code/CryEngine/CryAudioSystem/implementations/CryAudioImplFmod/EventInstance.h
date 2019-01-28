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
	explicit CEventInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const eventId,
		CEvent const* const pEvent,
		CBaseObject const* const pBaseObject)
		: m_triggerInstanceId(triggerInstanceId)
		, m_eventId(eventId)
		, m_state(EEventState::None)
		, m_lowpassFrequencyMax(0.0f)
		, m_lowpassFrequencyMin(0.0f)
		, m_pInstance(nullptr)
		, m_pMasterTrack(nullptr)
		, m_pLowpass(nullptr)
		, m_pOcclusionParameter(nullptr)
		, m_pAbsoluteVelocityParameter(nullptr)
		, m_pEvent(pEvent)
		, m_toBeRemoved(false)
		, m_pBaseObject(pBaseObject)
	{}
#else
	explicit CEventInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const eventId,
		CEvent const* const pEvent)
		: m_triggerInstanceId(triggerInstanceId)
		, m_eventId(eventId)
		, m_state(EEventState::None)
		, m_lowpassFrequencyMax(0.0f)
		, m_lowpassFrequencyMin(0.0f)
		, m_pInstance(nullptr)
		, m_pMasterTrack(nullptr)
		, m_pLowpass(nullptr)
		, m_pOcclusionParameter(nullptr)
		, m_pAbsoluteVelocityParameter(nullptr)
		, m_pEvent(pEvent)
		, m_toBeRemoved(false)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	~CEventInstance();

	TriggerInstanceId            GetTriggerInstanceId() const                                       { return m_triggerInstanceId; }
	uint32                       GetId() const                                                      { return m_eventId; }
	EEventState                  GetState() const                                                   { return m_state; }
	CEvent const*                GetEvent() const                                                   { return m_pEvent; }

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
	CBaseObject const* GetObject() const { return m_pBaseObject; }
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

private:

	TriggerInstanceId const          m_triggerInstanceId;
	uint32 const                     m_eventId;

	EEventState                      m_state;

	float                            m_lowpassFrequencyMax;
	float                            m_lowpassFrequencyMin;

	FMOD::Studio::EventInstance*     m_pInstance;
	FMOD::ChannelGroup*              m_pMasterTrack;
	FMOD::DSP*                       m_pLowpass;
	FMOD::Studio::ParameterInstance* m_pOcclusionParameter;
	FMOD::Studio::ParameterInstance* m_pAbsoluteVelocityParameter;

	CEvent const* const              m_pEvent;

	std::atomic_bool                 m_toBeRemoved;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	CBaseObject const* const m_pBaseObject;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
