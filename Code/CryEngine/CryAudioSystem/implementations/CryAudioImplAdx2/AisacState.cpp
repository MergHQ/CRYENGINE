// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AisacState.h"
#include "Object.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CAisacState::Set(IObject* const pIObject)
{
	auto const pObject = static_cast<CObject const*>(pIObject);

	CriAtomExPlayerHn const pPlayer = pObject->GetPlayer();
	criAtomExPlayer_SetAisacControlByName(pPlayer, static_cast<CriChar8 const*>(m_name.c_str()), m_value);
	criAtomExPlayer_UpdateAll(pPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CAisacState::SetGlobally()
{
	auto const szName = static_cast<CriChar8 const*>(m_name.c_str());

	for (auto const pObject : g_constructedObjects)
	{
		CriAtomExPlayerHn const pPlayer = pObject->GetPlayer();
		criAtomExPlayer_SetAisacControlByName(pPlayer, szName, m_value);
		criAtomExPlayer_UpdateAll(pPlayer);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
