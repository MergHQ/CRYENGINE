// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SoundEngine.h"
#include <CryCore/CryCrc32.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
inline const SampleId GetIDFromString(const string& name)
{
	return CCrc32::ComputeLowercase(name.c_str());
}

inline void GetDistanceAngleToObject(const CAudioObjectTransformation& listener, const CAudioObjectTransformation& object, float& out_distance, float& out_angle)
{
	const Vec3 listenerToObject = object.GetPosition() - listener.GetPosition();

	// Distance
	out_distance = listenerToObject.len();

	// Angle
	// Project point to plane formed by the listeners position/direction
	Vec3 n = listener.GetUp().GetNormalized();
	Vec3 objectDir = Vec3::CreateProjection(listenerToObject, n).normalized();

	// Get angle between listener position and projected point
	const Vec3 listenerDir = listener.GetForward().GetNormalizedFast();
	out_angle = RAD2DEG(asin_tpl(objectDir.Cross(listenerDir).Dot(n)));
}
}
}
}
