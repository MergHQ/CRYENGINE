// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
CImpl* g_pImpl = nullptr;
CGlobalObject* g_pObject = nullptr;
CListener* g_pListener = nullptr;
uint32 g_numObjectsWithDoppler = 0;
Objects g_constructedObjects;
EventToParameterIndexes g_eventToParameterIndexes;
SnapshotEventInstances g_activeSnapshots;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
ActiveSnapshots g_activeSnapshotNames;
VcaValues g_vcaValues;
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}      // namespace Fmod
}      // namespace Impl
}      // namespace CryAudio