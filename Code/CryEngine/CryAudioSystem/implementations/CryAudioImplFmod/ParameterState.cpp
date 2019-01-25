// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ParameterState.h"
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
CParameterState::~CParameterState()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->RemoveParameter(m_id);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameterState::Set(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
		pBaseObject->SetParameter(m_id, m_value);
	}
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object pointer passed to the Fmod implementation of %s", __FUNCTION__);
	}
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CParameterState::SetGlobally()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		Set(pBaseObject);
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
