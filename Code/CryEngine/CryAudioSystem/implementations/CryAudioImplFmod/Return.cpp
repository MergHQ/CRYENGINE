// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Return.h"
#include "Common.h"
#include "BaseObject.h"

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CReturn::~CReturn()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->RemoveReturn(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CReturn::Set(IObject* const pIObject, float const amount)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
		pBaseObject->SetReturn(this, amount);
	}
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object pointer passed to the Fmod implementation of %s", __FUNCTION__);
	}
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio