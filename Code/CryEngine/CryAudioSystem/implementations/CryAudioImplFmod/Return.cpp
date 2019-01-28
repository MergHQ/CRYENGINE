// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Return.h"
#include "Common.h"
#include "BaseObject.h"

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
	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
	pBaseObject->SetReturn(this, amount);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio