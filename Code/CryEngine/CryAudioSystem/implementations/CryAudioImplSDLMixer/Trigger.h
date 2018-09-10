// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include "GlobalData.h"
#include <ATLEntityData.h>
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

class CTrigger final : public ITrigger
{
public:

	CTrigger() = delete;
	CTrigger(CTrigger const&) = delete;
	CTrigger(CTrigger&&) = delete;
	CTrigger& operator=(CTrigger const&) = delete;
	CTrigger& operator=(CTrigger&&) = delete;

	explicit CTrigger(
		EEventType const type,
		SampleId const sampleId,
		float const attenuationMinDistance = s_defaultMinAttenuationDist,
		float const attenuationMaxDistance = s_defaultMaxAttenuationDist,
		int const volume = 128,
		int const numLoops = 1,
		int const fadeInTime = 0,
		int const fadeOutTime = 0,
		bool const isPanningEnabled = true);

	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITrigger
	virtual ERequestStatus Load() const override                             { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                           { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITrigger

	EEventType GetType() const                   { return m_type; }
	SampleId   GetSampleId() const               { return m_sampleId; }

	float      GetAttenuationMinDistance() const { return m_attenuationMinDistance; }
	float      GetAttenuationMaxDistance() const { return m_attenuationMaxDistance; }
	int        GetVolume() const                 { return m_volume; }
	int        GetNumLoops() const               { return m_numLoops; }
	int        GetFadeInTime() const             { return m_fadeInTime; }
	int        GetFadeOutTime() const            { return m_fadeOutTime; }
	bool       IsPanningEnabled() const          { return m_isPanningEnabled; }

private:

	EEventType const m_type;
	SampleId const   m_sampleId;
	float const      m_attenuationMinDistance;
	float const      m_attenuationMaxDistance;
	int const        m_volume;
	int const        m_numLoops;
	int const        m_fadeInTime;
	int const        m_fadeOutTime;
	bool const       m_isPanningEnabled;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio