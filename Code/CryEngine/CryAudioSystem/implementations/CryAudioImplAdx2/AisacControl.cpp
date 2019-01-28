// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AisacControl.h"
#include "BaseObject.h"

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CAisacControl::Set(IObject* const pIObject, float const value)
{
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

	CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
	criAtomExPlayer_SetAisacControlByName(pPlayer, static_cast<CriChar8 const*>(m_name.c_str()), static_cast<CriFloat32>(m_multiplier * value + m_shift));
	criAtomExPlayer_UpdateAll(pPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CAisacControl::SetGlobally(float const value)
{
	auto const finalValue = static_cast<CriFloat32>(m_multiplier * value + m_shift);
	auto const szName = static_cast<CriChar8 const*>(m_name.c_str());

	for (auto const pBaseObject : g_constructedObjects)
	{
		CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
		criAtomExPlayer_SetAisacControlByName(pPlayer, szName, finalValue);
		criAtomExPlayer_UpdateAll(pPlayer);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
