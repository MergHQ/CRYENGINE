// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ParameterEnvironment.h"
#include "Common.h"
#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CParameterEnvironment::~CParameterEnvironment()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->RemoveParameter(m_id);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameterEnvironment::Set(IObject* const pIObject, float const amount)
{
	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
	pBaseObject->SetParameter(m_id, m_multiplier * amount + m_shift);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio