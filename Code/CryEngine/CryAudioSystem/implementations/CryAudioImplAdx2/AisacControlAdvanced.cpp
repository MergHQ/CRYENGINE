// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AisacControlAdvanced.h"
#include "Object.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CAisacControlAdvanced::Set(IObject* const pIObject, float const value)
{
	auto const pObject = static_cast<CObject const*>(pIObject);

	CriAtomExPlayerHn const pPlayer = pObject->GetPlayer();
	criAtomExPlayer_SetAisacControlById(pPlayer, m_id, static_cast<CriFloat32>(m_multiplier * NormalizeValue(value, m_minValue, m_maxValue) + m_shift));
	criAtomExPlayer_UpdateAll(pPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CAisacControlAdvanced::SetGlobally(float const value)
{
	auto const finalValue = static_cast<CriFloat32>(m_multiplier * NormalizeValue(value, m_minValue, m_maxValue) + m_shift);

	for (auto const pObject : g_constructedObjects)
	{
		CriAtomExPlayerHn const pPlayer = pObject->GetPlayer();
		criAtomExPlayer_SetAisacControlById(pPlayer, m_id, finalValue);
		criAtomExPlayer_UpdateAll(pPlayer);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
