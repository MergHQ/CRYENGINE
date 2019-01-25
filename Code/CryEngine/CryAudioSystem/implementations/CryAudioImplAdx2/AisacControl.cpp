// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AisacControl.h"
#include "BaseObject.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

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
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

		CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
		criAtomExPlayer_SetAisacControlByName(pPlayer, static_cast<CriChar8 const*>(m_name), static_cast<CriFloat32>(m_multiplier * value + m_shift));
		criAtomExPlayer_UpdateAll(pPlayer);
	}
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid Object passed to the Adx2 implementation of %s", __FUNCTION__);
	}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAisacControl::SetGlobally(float const value)
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		Set(pBaseObject, value);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
