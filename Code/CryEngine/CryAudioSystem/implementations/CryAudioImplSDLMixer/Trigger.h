// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include "GlobalData.h"
#include <ITriggerConnection.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
enum class EEventType : EnumFlagsType
{
	None,
	Start,
	Stop,
	Pause,
	Resume,
};

class CTrigger final : public ITriggerConnection, public CPoolObject<CTrigger, stl::PSyncNone>
{
public:

	CTrigger() = delete;
	CTrigger(CTrigger const&) = delete;
	CTrigger(CTrigger&&) = delete;
	CTrigger& operator=(CTrigger const&) = delete;
	CTrigger& operator=(CTrigger&&) = delete;

	explicit CTrigger(
		uint32 const id,
		EEventType const type,
		SampleId const sampleId,
		float const attenuationMinDistance = s_defaultMinAttenuationDist,
		float const attenuationMaxDistance = s_defaultMaxAttenuationDist,
		int const volume = 128,
		int const numLoops = 1,
		int const fadeInTime = 0,
		int const fadeOutTime = 0,
		bool const isPanningEnabled = true)
		: m_id(id)
		, m_type(type)
		, m_sampleId(sampleId)
		, m_attenuationMinDistance(attenuationMinDistance)
		, m_attenuationMaxDistance(attenuationMaxDistance)
		, m_volume(volume)
		, m_numLoops(numLoops)
		, m_fadeInTime(fadeInTime)
		, m_fadeOutTime(fadeOutTime)
		, m_isPanningEnabled(isPanningEnabled)
	{}

	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ERequestStatus Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	virtual ERequestStatus Load() const override                                                 { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                               { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(TriggerInstanceId const triggerInstanceId) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(TriggerInstanceId const triggerInstanceId) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITriggerConnection

	uint32     GetId() const       { return m_id; }
	EEventType GetType() const     { return m_type; }
	SampleId   GetSampleId() const { return m_sampleId; }

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	void        SetName(char const* const szName) { m_name = szName; }
	char const* GetName() const                   { return m_name.c_str(); }
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	float GetAttenuationMinDistance() const { return m_attenuationMinDistance; }
	float GetAttenuationMaxDistance() const { return m_attenuationMaxDistance; }
	int   GetVolume() const                 { return m_volume; }
	int   GetNumLoops() const               { return m_numLoops; }
	int   GetFadeInTime() const             { return m_fadeInTime; }
	int   GetFadeOutTime() const            { return m_fadeOutTime; }
	bool  IsPanningEnabled() const          { return m_isPanningEnabled; }

private:

	uint32 const     m_id;
	EEventType const m_type;
	SampleId const   m_sampleId;
	float const      m_attenuationMinDistance;
	float const      m_attenuationMaxDistance;
	int const        m_volume;
	int const        m_numLoops;
	int const        m_fadeInTime;
	int const        m_fadeOutTime;
	bool const       m_isPanningEnabled;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio