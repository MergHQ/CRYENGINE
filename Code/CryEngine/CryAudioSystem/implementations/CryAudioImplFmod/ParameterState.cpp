// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ParameterState.h"
#include "Common.h"
#include "BaseObject.h"

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
	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
	pBaseObject->SetParameter(m_id, m_value);
}

//////////////////////////////////////////////////////////////////////////
void CParameterState::SetGlobally()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->SetParameter(m_id, m_value);
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
