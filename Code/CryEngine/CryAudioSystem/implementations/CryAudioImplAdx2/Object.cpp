// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"

#include "Event.h"
#include "Parameter.h"
#include "SwitchState.h"
#include "Environment.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	auto const pEnvironment = static_cast<CEnvironment const*>(pIEnvironment);

	if (pEnvironment != nullptr)
	{
		if (pEnvironment->GetType() == EEnvironmentType::Bus)
		{
			criAtomExPlayer_SetBusSendLevelByName(m_pPlayer, pEnvironment->GetName(), static_cast<CriFloat32>(amount));
			criAtomExPlayer_UpdateAll(m_pPlayer);
		}
		else
		{
			criAtomExPlayer_SetAisacControlByName(m_pPlayer, pEnvironment->GetName(), static_cast<CriFloat32>(pEnvironment->GetMultiplier() * amount + pEnvironment->GetShift()));
			criAtomExPlayer_UpdateAll(m_pPlayer);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	auto const pParameter = static_cast<CParameter const*>(pIParameter);

	if (pParameter != nullptr)
	{
		EParameterType const type = pParameter->GetType();

		switch (type)
		{
		case EParameterType::AisacControl:
			{
				criAtomExPlayer_SetAisacControlByName(m_pPlayer, pParameter->GetName(), static_cast<CriFloat32>(pParameter->GetMultiplier() * value + pParameter->GetValueShift()));
				criAtomExPlayer_UpdateAll(m_pPlayer);
			}
			break;
		case EParameterType::Category:
			{
				criAtomExCategory_SetVolumeByName(pParameter->GetName(), static_cast<CriFloat32>(pParameter->GetMultiplier() * value + pParameter->GetValueShift()));
			}
			break;
		case EParameterType::GameVariable:
			{
				criAtomEx_SetGameVariableByName(pParameter->GetName(), static_cast<CriFloat32>(pParameter->GetMultiplier() * value + pParameter->GetValueShift()));
			}
			break;
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown EParameterType: %" PRISIZE_T, type);
			}
			break;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid Parameter Data passed to the Adx2 implementation of SetParameter");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	auto const pSwitchState = static_cast<CSwitchState const*>(pISwitchState);

	if (pSwitchState != nullptr)
	{
		ESwitchType const type = pSwitchState->GetType();

		switch (type)
		{
		case ESwitchType::Selector:
			{
				criAtomExPlayer_SetSelectorLabel(m_pPlayer, pSwitchState->GetName(), pSwitchState->GetLabelName());
				criAtomExPlayer_UpdateAll(m_pPlayer);
			}
			break;
		case ESwitchType::AisacControl:
			{
				criAtomExPlayer_SetAisacControlByName(m_pPlayer, pSwitchState->GetName(), pSwitchState->GetValue());
				criAtomExPlayer_UpdateAll(m_pPlayer);
			}
			break;
		case ESwitchType::Category:
			{
				criAtomExCategory_SetVolumeByName(pSwitchState->GetName(), pSwitchState->GetValue());
			}
			break;
		case ESwitchType::GameVariable:
			{
				criAtomEx_SetGameVariableByName(pSwitchState->GetName(), pSwitchState->GetValue());
			}
			break;
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown ESwitchType: %" PRISIZE_T, type);
			}
			break;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid SwitchState Data passed to the Adx2 implementation of SetSwitchState");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	criAtomExPlayer_SetAisacControlByName(m_pPlayer, static_cast<CriChar8 const*>("occlusion"), static_cast<CriFloat32>(occlusion));
	criAtomExPlayer_UpdateAll(m_pPlayer);

	m_obstruction = obstruction;
	m_occlusion = occlusion;
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
