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
class CEnvironment;
class CBaseObject;
class CTrigger;

enum class EEventState : EnumFlagsType
{
	None,
	Playing,
	Virtual,
};

class CEvent final : public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(TriggerInstanceId const triggerInstanceId)
		: m_triggerInstanceId(triggerInstanceId)
		, m_id(InvalidCRC32)
		, m_state(EEventState::None)
		, m_lowpassFrequencyMax(0.0f)
		, m_lowpassFrequencyMin(0.0f)
		, m_pInstance(nullptr)
		, m_pMasterTrack(nullptr)
		, m_pLowpass(nullptr)
		, m_pOcclusionParameter(nullptr)
		, m_pAbsoluteVelocityParameter(nullptr)
		, m_pObject(nullptr)
		, m_pTrigger(nullptr)
		, m_toBeRemoved(false)
	{}

	~CEvent();

	bool                         PrepareForOcclusion();
	void                         SetOcclusion(float const occlusion);

	TriggerInstanceId            GetTriggerInstanceId() const                              { return m_triggerInstanceId; }

	uint32                       GetId() const                                             { return m_id; }
	void                         SetId(uint32 const id)                                    { m_id = id; }

	FMOD::Studio::EventInstance* GetInstance() const                                       { return m_pInstance; }
	void                         SetInstance(FMOD::Studio::EventInstance* const pInstance) { m_pInstance = pInstance; }

	void                         SetObject(CBaseObject* const pAudioObject)                { m_pObject = pAudioObject; }

	CTrigger const*              GetTrigger() const                                        { return m_pTrigger; }
	void                         SetTrigger(CTrigger const* const pTrigger)                { m_pTrigger = pTrigger; }

	bool                         HasAbsoluteVelocityParameter() const                      { return m_pAbsoluteVelocityParameter != nullptr; }
	void                         SetInternalParameters();

	EEventState                  GetState() const { return m_state; }

	void                         TrySetEnvironment(CEnvironment const* const pEnvironment, float const value);
	void                         UpdateVirtualState();
	void                         SetAbsoluteVelocity(float const value);
	void                         StopAllowFadeOut();
	void                         StopImmediate();

	void                         SetToBeRemoved()      { m_toBeRemoved = true; }
	bool                         IsToBeRemoved() const { return m_toBeRemoved; }

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	void         SetName(char const* const szName) { m_name = szName; }
	char const*  GetName() const                   { return m_name.c_str(); }
	CBaseObject* GetObject() const                 { return m_pObject; }
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

private:

	TriggerInstanceId const          m_triggerInstanceId;
	uint32                           m_id;

	EEventState                      m_state;

	float                            m_lowpassFrequencyMax;
	float                            m_lowpassFrequencyMin;

	FMOD::Studio::EventInstance*     m_pInstance;
	FMOD::ChannelGroup*              m_pMasterTrack;
	FMOD::DSP*                       m_pLowpass;
	FMOD::Studio::ParameterInstance* m_pOcclusionParameter;
	FMOD::Studio::ParameterInstance* m_pAbsoluteVelocityParameter;
	CBaseObject*                     m_pObject;
	CTrigger const*                  m_pTrigger;
	std::atomic_bool                 m_toBeRemoved;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
