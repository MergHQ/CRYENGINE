// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <SharedAudioData.h>
#include <fmod_common.h>

#define FMOD_IMPL_INFO_STRING             "Fmod Studio - "

#define ASSERT_FMOD_OK                    CRY_ASSERT(fmodResult == FMOD_OK)
#define ASSERT_FMOD_OK_OR_INVALID_HANDLE  CRY_ASSERT(fmodResult == FMOD_OK || fmodResult == FMOD_ERR_INVALID_HANDLE)
#define ASSERT_FMOD_OK_OR_NOT_LOADED      CRY_ASSERT(fmodResult == FMOD_OK || fmodResult == FMOD_ERR_STUDIO_NOT_LOADED)
#define ASSERT_FMOD_OK_OR_EVENT_NOT_FOUND CRY_ASSERT(fmodResult == FMOD_OK || fmodResult == FMOD_ERR_EVENT_NOTFOUND)
#define FMOD_IMPL_INVALID_INDEX           (-1)

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
///////////////////////////////////////////////////////////////////////////
inline void FillFmodObjectPosition(CObjectTransformation const& transformation, FMOD_3D_ATTRIBUTES& outAttributes)
{
	outAttributes.forward.x = transformation.GetForward().x;
	outAttributes.forward.z = transformation.GetForward().y;
	outAttributes.forward.y = transformation.GetForward().z;

	outAttributes.position.x = transformation.GetPosition().x;
	outAttributes.position.z = transformation.GetPosition().y;
	outAttributes.position.y = transformation.GetPosition().z;

	outAttributes.up.x = transformation.GetUp().x;
	outAttributes.up.z = transformation.GetUp().y;
	outAttributes.up.y = transformation.GetUp().z;

	// Use CE object parameters "absolute_velocity" and "relative_velocity" instead.
	outAttributes.velocity.x = 0.0f;
	outAttributes.velocity.z = 0.0f;
	outAttributes.velocity.y = 0.0f;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
