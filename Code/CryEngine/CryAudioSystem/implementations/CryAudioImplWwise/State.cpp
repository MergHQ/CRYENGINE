// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "State.h"
#include "Object.h"

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include "BaseObject.h"
	#include <Logger.h>
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
void CState::Set(IObject* const pIObject)
{
	SetGlobally();

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	auto const pObject = static_cast<CBaseObject*>(pIObject);
	Cry::Audio::Log(ELogType::Warning, R"(Wwise - state "%s: %s" was set on object "%s". Consider setting it globally.)",
	                m_stateGroupName.c_str(), m_stateName.c_str(), pObject->GetName());
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CState::SetGlobally()
{
	AK::SoundEngine::SetState(m_stateGroupId, m_stateId);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	g_debugStates[m_stateGroupName] = m_stateName;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
