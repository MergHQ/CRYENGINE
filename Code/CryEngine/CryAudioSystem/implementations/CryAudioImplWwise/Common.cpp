// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"

#include "Impl.h"

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include "Event.h"
	#include "Object.h"
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
CImpl* g_pImpl = nullptr;
CListener* g_pListener = nullptr;
CObject* g_pObject = nullptr;

AkGameObjectID g_listenerId = AK_INVALID_GAME_OBJECT; // To be removed once multi-listener support is implemented.
AkGameObjectID g_globalObjectId = AK_INVALID_GAME_OBJECT;

uint32 g_numObjectsWithRelativeVelocity = 0;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
CryCriticalSection g_cs;
std::unordered_map<AkPlayingID, CEvent*> g_playingIds;
std::unordered_map<AkGameObjectID, CObject*> g_gameObjectIds;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}      // namespace Wwise
}      // namespace Impl
}      // namespace CryAudio
