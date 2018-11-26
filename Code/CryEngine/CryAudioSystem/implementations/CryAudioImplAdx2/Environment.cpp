// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Environment.h"

#include "Object.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CEnvironment::Set(IObject* const pIObject, float const amount)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

		switch (m_type)
		{
		case EEnvironmentType::Bus:
			{
				CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
				criAtomExPlayer_SetBusSendLevelByName(pPlayer, GetName(), static_cast<CriFloat32>(amount));
				criAtomExPlayer_UpdateAll(pPlayer);

				break;
			}
		case EEnvironmentType::AisacControl:
			{
				CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
				criAtomExPlayer_SetAisacControlByName(pPlayer, GetName(), static_cast<CriFloat32>(m_multiplier * amount + m_shift));
				criAtomExPlayer_UpdateAll(pPlayer);

				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown EEnvironmentType: %" PRISIZE_T " of %s", m_type, m_name.c_str());

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid Object passed to the Adx2 implementation of %s", __FUNCTION__);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
