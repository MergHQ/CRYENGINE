// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchState.h"
#include "Object.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
void CSwitchState::Set(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		pObject->SetVolume(m_sampleId, m_value);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSwitchState::SetGlobally()
{
	for (auto const pObject : g_objects)
	{
		Set(pObject);
	}
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
