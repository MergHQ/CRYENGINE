// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AisacState.h"
#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CAisacState::Set(IObject* const pIObject)
{
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

	CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
	criAtomExPlayer_SetAisacControlByName(pPlayer, static_cast<CriChar8 const*>(m_name.c_str()), m_value);
	criAtomExPlayer_UpdateAll(pPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CAisacState::SetGlobally()
{
	auto const szName = static_cast<CriChar8 const*>(m_name.c_str());

	for (auto const pBaseObject : g_constructedObjects)
	{
		CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
		criAtomExPlayer_SetAisacControlByName(pPlayer, szName, m_value);
		criAtomExPlayer_UpdateAll(pPlayer);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
