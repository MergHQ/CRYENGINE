// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"
#include "Common.h"
#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CParameter::~CParameter()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->RemoveParameter(m_id);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameter::Set(IObject* const pIObject, float const value)
{
	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
	pBaseObject->SetParameter(m_id, m_multiplier * value + m_shift);
}

//////////////////////////////////////////////////////////////////////////
void CParameter::SetGlobally(float const value)
{
	float const finalValue = m_multiplier * value + m_shift;

	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->SetParameter(m_id, finalValue);
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio