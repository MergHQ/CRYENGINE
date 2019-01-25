// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AuxBus.h"
#include "Object.h"

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
void CAuxBus::Set(IObject* const pIObject, float const amount)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		pObject->SetAuxBusSend(m_busId, amount);
	}
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid object passed to the Wwise implementation of %s", __FUNCTION__);
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
