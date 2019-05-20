// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"
#include "Common.h"

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	#include <Logger.h>
#endif // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CGlobalObject::CGlobalObject(int const listenerMask, Listeners const& listeners)
	: CBaseObject(listenerMask, listeners)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	m_name = "Global Object";
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusion(IListener* const pIListener, float const occlusion, uint8 const numRemainingListeners)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion and obstruction values on the global object!");
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusionType(EOcclusionType const occlusionType)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion type on the global object!");
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	if (enable)
	{
		char const* szType = nullptr;

		switch (type)
		{
		case EObjectFunctionality::TrackAbsoluteVelocity:
			{
				szType = "absolute velocity tracking";

				break;
			}
		case EObjectFunctionality::TrackRelativeVelocity:
			{
				szType = "relative velocity tracking";

				break;
			}
		default:
			{
				szType = "an undefined functionality";

				break;
			}
		}

		Cry::Audio::Log(ELogType::Error, "Trying to enable %s on the global object!", szType);
	}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}
}      // namespace Fmod
}      // namespace Impl
}      // namespace CryAudio