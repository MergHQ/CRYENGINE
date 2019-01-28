// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VcaState.h"

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include "BaseObject.h"
	#include <Logger.h>
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

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

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);
	Cry::Audio::Log(ELogType::Warning, R"(Fmod - VCA "%s" was set to %f on object "%s". Consider setting it globally.)", m_name.c_str(), m_value, pBaseObject->GetName());
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CVcaState::SetGlobally()
{
	m_pVca->setVolume(m_value);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	g_vcaValues[m_name] = m_value;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
