// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchState.h"
#include "BaseObject.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CSwitchState::Set(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

		switch (m_type)
		{
		case ESwitchType::Selector:
			{
				CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
				criAtomExPlayer_SetSelectorLabel(pPlayer, GetName(), static_cast<CriChar8 const*>(m_labelName));
				criAtomExPlayer_UpdateAll(pPlayer);

				break;
			}
		case ESwitchType::AisacControl:
			{
				CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
				criAtomExPlayer_SetAisacControlByName(pPlayer, static_cast<CriChar8 const*>(m_name), m_value);
				criAtomExPlayer_UpdateAll(pPlayer);

				break;
			}
		case ESwitchType::Category:
			{
				criAtomExCategory_SetVolumeByName(GetName(), m_value);

				break;
			}
		case ESwitchType::GameVariable:
			{
				criAtomEx_SetGameVariableByName(GetName(), m_value);

				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown ESwitchType: %" PRISIZE_T " of %s", m_type, m_name.c_str());

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid SwitchState Data passed to the Adx2 implementation of %s", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSwitchState::SetGlobally()
{
	switch (m_type)
	{
	case ESwitchType::Selector: // Intentional fall-through.
	case ESwitchType::AisacControl:
		{
			for (auto const pObject : g_constructedObjects)
			{
				Set(pObject);
			}

			break;
		}
	case ESwitchType::Category:
		{
			criAtomExCategory_SetVolumeByName(GetName(), m_value);

			break;
		}
	case ESwitchType::GameVariable:
		{
			criAtomEx_SetGameVariableByName(GetName(), m_value);

			break;
		}
	default:
		{
			Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown ESwitchType: %" PRISIZE_T " of %s", m_type, m_name.c_str());

			break;
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
