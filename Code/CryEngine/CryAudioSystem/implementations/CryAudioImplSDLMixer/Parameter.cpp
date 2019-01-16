// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"
#include "Object.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
void CParameter::Set(IObject* const pIObject, float const value)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		float parameterValue = m_multiplier * crymath::clamp(value, 0.0f, 1.0f) + m_shift;
		parameterValue = crymath::clamp(parameterValue, 0.0f, 1.0f);
		pObject->SetVolume(m_sampleId, parameterValue);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameter::SetGlobally(float const value)
{
	for (auto const pObject : g_objects)
	{
		Set(pObject, value);
	}
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
