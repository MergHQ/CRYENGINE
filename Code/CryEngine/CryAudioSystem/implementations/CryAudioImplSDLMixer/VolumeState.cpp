// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VolumeState.h"
#include "Object.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
void CVolumeState::Set(IObject* const pIObject)
{
	auto const pObject = static_cast<CObject*>(pIObject);
	pObject->SetVolume(m_sampleId, m_value);
}

//////////////////////////////////////////////////////////////////////////
void CVolumeState::SetGlobally()
{
	for (auto const pObject : g_objects)
	{
		pObject->SetVolume(m_sampleId, m_value);
	}
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
