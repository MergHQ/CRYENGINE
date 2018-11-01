// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <IEvent.h>
#include <PoolObject.h>

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
class CEvent;

namespace Impl
{
namespace Fmod
{
class CEnvironment;
class CBaseObject;
class CTrigger;

class CEvent final : public IEvent, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(CryAudio::CEvent* const pEvent)
		: m_pEvent(pEvent)
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
	{}

	virtual ~CEvent() override;

	// CryAudio::Impl::IEvent
	virtual ERequestStatus Stop() override;
	// ~CryAudio::Impl::IEvent

	bool                         PrepareForOcclusion();
	void                         SetOcclusion(float const occlusion);
	CryAudio::CEvent&            GetEvent() const                                          { return *m_pEvent; }

	uint32                       GetId() const                                             { return m_id; }
	void                         SetId(uint32 const id)                                    { m_id = id; }

	FMOD::Studio::EventInstance* GetInstance() const                                       { return m_pInstance; }
	void                         SetInstance(FMOD::Studio::EventInstance* const pInstance) { m_pInstance = pInstance; }

	CBaseObject* const           GetObject()                                               { return m_pObject; }
	void                         SetObject(CBaseObject* const pAudioObject)                { m_pObject = pAudioObject; }

	CTrigger const*              GetTrigger() const                                        { return m_pTrigger; }
	void                         SetTrigger(CTrigger const* const pTrigger)                { m_pTrigger = pTrigger; }

	bool                         HasAbsoluteVelocityParameter() const                      { return m_pAbsoluteVelocityParameter != nullptr; }
	void                         SetAbsoluteVelocityParameter();

	EEventState                  GetState() const { return m_state; }

	void                         TrySetEnvironment(CEnvironment const* const pEnvironment, float const value);
	void                         UpdateVirtualState();
	void                         SetAbsoluteVelocity(float const value);

private:

	CryAudio::CEvent*                m_pEvent;
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
};

using Events = std::vector<CEvent*>;
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
