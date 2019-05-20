// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"
#include "Common.h"

#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	#include <Logger.h>
#endif // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
CGlobalObject::CGlobalObject(CListener* const pListener)
	: CBaseObject(pListener)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	m_name = "Global Object";
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetTransformation(CTransformation const& transformation)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set transformation on the global object!");
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusion(IListener* const pIListener, float const occlusion, uint8 const numRemainingListeners)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion and obstruction values on the global object!");
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusionType(EOcclusionType const occlusionType)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion type on the global object!");
#endif  // CRY_AUDIO_IMPL_ADX2_USE_DEBUG_CODE
}
}      // namespace Adx2
}      // namespace Impl
}      // namespace CryAudio
