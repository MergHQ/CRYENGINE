// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "Object.h"
#include "Event.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

#if defined(CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE)
	#include <Logger.h>
#endif  // CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE

#include <SDL_mixer.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
bool g_bMuted = false;
CImpl* g_pImpl = nullptr;
Objects g_objects;

//////////////////////////////////////////////////////////////////////////
int GetAbsoluteVolume(int const eventVolume, float const multiplier)
{
	int absoluteVolume = static_cast<int>(static_cast<float>(eventVolume) * multiplier);
	absoluteVolume = crymath::clamp(absoluteVolume, 0, 128);
	return absoluteVolume;
}

//////////////////////////////////////////////////////////////////////////
float GetVolumeMultiplier(CObject* const pObject, SampleId const sampleId)
{
	float volumeMultiplier = 1.0f;
	auto const volumeMultiplierPair = pObject->m_volumeMultipliers.find(sampleId);

	if (volumeMultiplierPair != pObject->m_volumeMultipliers.end())
	{
		volumeMultiplier = volumeMultiplierPair->second;
	}

	return volumeMultiplier;
}

//////////////////////////////////////////////////////////////////////////
void GetDistanceAngleToObject(
	CTransformation const& listenerTransformation,
	CTransformation const& objectTransformation,
	float& distance,
	float& angle)
{
	Vec3 const listenerToObject = objectTransformation.GetPosition() - listenerTransformation.GetPosition();

	// Distance
	distance = listenerToObject.len();

	// Angle
	// Project point to plane formed by the listeners position/direction
	Vec3 n = listenerTransformation.GetUp().GetNormalized();
	Vec3 objectDir = Vec3::CreateProjection(listenerToObject, n).normalized();

	// Get angle between listener position and projected point
	Vec3 const listenerDir = listenerTransformation.GetForward().GetNormalizedFast();
	angle = RAD2DEG(asin_tpl(objectDir.Cross(listenerDir).Dot(n)));
}

//////////////////////////////////////////////////////////////////////////
void SetChannelPosition(CEvent const& event, int const channelID, float const distance, float const angle)
{
	static const uint8 sdlMaxDistance = 255;
	float const min = event.GetAttenuationMinDistance();
	float const max = event.GetAttenuationMaxDistance();

	if (min <= max)
	{
		uint8 nDistance = 0;

		if ((max >= 0.0f) && (distance > min))
		{
			if (min != max)
			{
				float const finalDistance = distance - min;
				float const range = max - min;
				nDistance = static_cast<uint8>((std::min((finalDistance / range), 1.0f) * sdlMaxDistance) + 0.5f);
			}
			else
			{
				nDistance = sdlMaxDistance;
			}
		}
		//Temp code, to be reviewed during the SetChannelPosition rewrite:
		Mix_SetDistance(channelID, nDistance);

		if (event.IsPanningEnabled())
		{
			//Temp code, to be reviewed during the SetChannelPosition rewrite:
			float const absAngle = fabs(angle);
			float const frontAngle = (angle > 0.0f ? 1.0f : -1.0f) * (absAngle > 90.0f ? 180.f - absAngle : absAngle);
			float const rightVolume = (frontAngle + 90.0f) / 180.0f;
			float const leftVolume = 1.0f - rightVolume;
			Mix_SetPanning(channelID,
			               static_cast<uint8>(255.0f * leftVolume),
			               static_cast<uint8>(255.0f * rightVolume));
		}
	}
#if defined(CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "The minimum attenuation distance (%f) is higher than the maximum (%f) of %s", min, max, event.GetName());
	}
#endif  // CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE
}

#if defined(CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE)
size_t g_loadedSampleSize = 0;
#endif // CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE
}      // namespace SDL_mixer
}      // namespace Impl
}      // namespace CryAudio
