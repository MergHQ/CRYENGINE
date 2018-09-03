// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>
#include <PoolObject.h>

// Fmod forward declarations.
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
class CATLEvent;

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

	explicit CEvent(CATLEvent* const pEvent);
	virtual ~CEvent() override;

	// CryAudio::Impl::IEvent
	virtual ERequestStatus Stop() override;
	// ~CryAudio::Impl::IEvent

	bool                         PrepareForOcclusion();
	void                         SetOcclusion(float const occlusion);
	CATLEvent&                   GetATLEvent() const                                       { return *m_pEvent; }
	uint32                       GetId() const                                             { return m_id; }
	void                         SetId(uint32 const id)                                    { m_id = id; }
	FMOD::Studio::EventInstance* GetInstance() const                                       { return m_pInstance; }
	void                         SetInstance(FMOD::Studio::EventInstance* const pInstance) { m_pInstance = pInstance; }
	CBaseObject* const           GetObject()                                               { return m_pObject; }
	void                         SetObject(CBaseObject* const pAudioObject)                { m_pObject = pAudioObject; }
	void                         TrySetEnvironment(CEnvironment const* const pEnvironment, float const value);

	void                         SetTrigger(CTrigger const* const pTrigger) { m_pTrigger = pTrigger; }
	CTrigger const*              GetTrigger() const                         { return m_pTrigger; }

	EEventState                  GetState() const                           { return m_state; }

	void                         UpdateVirtualState();

private:

	CATLEvent*                       m_pEvent;
	uint32                           m_id;

	EEventState                      m_state;

	float                            m_lowpassFrequencyMax;
	float                            m_lowpassFrequencyMin;

	FMOD::Studio::EventInstance*     m_pInstance;
	FMOD::ChannelGroup*              m_pMasterTrack;
	FMOD::DSP*                       m_pLowpass;
	FMOD::Studio::ParameterInstance* m_pOcclusionParameter;
	CBaseObject*                     m_pObject;
	CTrigger const*                  m_pTrigger;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
