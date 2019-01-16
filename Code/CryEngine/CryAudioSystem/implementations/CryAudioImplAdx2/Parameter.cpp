// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"
#include "BaseObject.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CParameter::Set(IObject* const pIObject, float const value)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

		switch (m_type)
		{
		case EParameterType::AisacControl:
			{
				CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
				criAtomExPlayer_SetAisacControlByName(pPlayer, GetName(), static_cast<CriFloat32>(m_multiplier * value + m_shift));
				criAtomExPlayer_UpdateAll(pPlayer);

				break;
			}
		case EParameterType::Category:
			{
				criAtomExCategory_SetVolumeByName(GetName(), static_cast<CriFloat32>(m_multiplier * value + m_shift));

				break;
			}
		case EParameterType::GameVariable:
			{
				criAtomEx_SetGameVariableByName(GetName(), static_cast<CriFloat32>(m_multiplier * value + m_shift));

				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown EParameterType: %" PRISIZE_T " of %s", m_type, m_name.c_str());

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid Object passed to the Adx2 implementation of %s", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameter::SetGlobally(float const value)
{
	switch (m_type)
	{
	case EParameterType::AisacControl:
		{
			for (auto const pObject : g_constructedObjects)
			{
				Set(pObject, value);
			}

			break;
		}
	case EParameterType::Category:
		{
			criAtomExCategory_SetVolumeByName(GetName(), static_cast<CriFloat32>(m_multiplier * value + m_shift));

			break;
		}
	case EParameterType::GameVariable:
		{
			criAtomEx_SetGameVariableByName(GetName(), static_cast<CriFloat32>(m_multiplier * value + m_shift));

			break;
		}
	default:
		{
			Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown EParameterType: %" PRISIZE_T " of %s", m_type, m_name.c_str());

			break;
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
