// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VcaState.h"

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	#include "BaseObject.h"
	#include <Logger.h>
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
void CVcaState::Set(IObject* const pIObject)
{
	SetGlobally();

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);
	Cry::Audio::Log(ELogType::Warning, R"(Fmod - VCA "%s" was set to %f on object "%s". Consider setting it globally.)", m_name.c_str(), m_value, pBaseObject->GetName());
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CVcaState::SetGlobally()
{
	m_pVca->setVolume(m_value);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	g_vcaValues[m_name] = m_value;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
