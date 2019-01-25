// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Switch.h"
#include "BaseObject.h"

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
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
void CSwitch::Set(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

		AK::SoundEngine::SetSwitch(m_switchGroupId, m_switchId, pBaseObject->GetId());
	}
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid object passed to the Wwise implementation of %s", __FUNCTION__);
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSwitch::SetGlobally()
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	Cry::Audio::Log(ELogType::Warning, "Wwise - Switches cannot get set globally!");
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
