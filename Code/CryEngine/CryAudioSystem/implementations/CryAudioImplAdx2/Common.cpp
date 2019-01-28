// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
CImpl* g_pImpl = nullptr;
CGlobalObject* g_pObject = nullptr;
CListener* g_pListener = nullptr;

Objects g_constructedObjects;
AcbHandles g_acbHandles;
CriAtomExPlayerConfig g_playerConfig;
CriAtomEx3dSourceConfig g_3dSourceConfig;

uint32 g_numObjectsWithDoppler = 0;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
GameVariableValues g_gameVariableValues;
CategoryValues g_categoryValues;
#endif // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}      // namespace Adx2
}      // namespace Impl
}      // namespace CryAudio