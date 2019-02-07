// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
FMOD::System* g_pLowLevelSystem = nullptr;
FMOD::Studio::System* g_pSystem = nullptr;

CImpl* g_pImpl = nullptr;
CGlobalObject* g_pObject = nullptr;
CListener* g_pListener = nullptr;
uint32 g_numObjectsWithDoppler = 0;
Objects g_constructedObjects;
EventToParameterIndexes g_eventToParameterIndexes;
SnapshotEventInstances g_activeSnapshots;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
ActiveSnapshots g_activeSnapshotNames;
VcaValues g_vcaValues;
#endif // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
}      // namespace Fmod
}      // namespace Impl
}      // namespace CryAudio