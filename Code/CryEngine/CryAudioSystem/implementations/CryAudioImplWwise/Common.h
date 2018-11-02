// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AK/AkWwiseSDKVersion.h"

#if AK_WWISESDK_VERSION_MAJOR <= 2017 && AK_WWISESDK_VERSION_MINOR < 2
	#error This version of Wwise is not supported, the minimum supported version is 2017.2.0
#endif

#include "AK/SoundEngine/Common/AkTypes.h"
#include <CryAudio/IAudioSystem.h>

#define WWISE_IMPL_INFO_STRING "Wwise " AK_WWISESDK_VERSIONNAME

#define ASSERT_WWISE_OK(x) (CRY_ASSERT(x == AK_Success))
#define IS_WWISE_OK(x)     (x == AK_Success)

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include <CryThreading/CryThread.h>
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CImpl;
class CListener;

extern CImpl* g_pImpl;
extern CListener* g_pListener;

extern uint32 g_numObjectsWithRelativeVelocity;

//////////////////////////////////////////////////////////////////////////
inline void FillAKVector(Vec3 const& vCryVector, AkVector& vAKVector)
{
	vAKVector.X = vCryVector.x;
	vAKVector.Y = vCryVector.z;
	vAKVector.Z = vCryVector.y;
}

///////////////////////////////////////////////////////////////////////////
inline void FillAKObjectPosition(CTransformation const& transformation, AkSoundPosition& outTransformation)
{
	AkVector vec1, vec2;
	FillAKVector(transformation.GetPosition(), vec1);
	outTransformation.SetPosition(vec1);
	FillAKVector(transformation.GetForward(), vec1);
	FillAKVector(transformation.GetUp(), vec2);
	outTransformation.SetOrientation(vec1, vec2);
}

///////////////////////////////////////////////////////////////////////////
inline void FillAKListenerPosition(CTransformation const& transformation, AkListenerPosition& outTransformation)
{
	AkVector vec1, vec2;
	FillAKVector(transformation.GetPosition(), vec1);
	outTransformation.SetPosition(vec1);
	FillAKVector(transformation.GetForward(), vec1);
	FillAKVector(transformation.GetUp(), vec2);
	outTransformation.SetOrientation(vec1, vec2);
}

extern AkGameObjectID g_listenerId; // To be removed once multi-listener support is implemented.
extern AkGameObjectID g_globalObjectId;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
class CEvent;
class CObject;
extern CryCriticalSection g_cs;
extern std::unordered_map<AkPlayingID, CEvent*> g_playingIds;
extern std::unordered_map<AkGameObjectID, CObject*> g_gameObjectIds;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}      // namespace Wwise
}      // namespace Impl
}      // namespace CryAudio
