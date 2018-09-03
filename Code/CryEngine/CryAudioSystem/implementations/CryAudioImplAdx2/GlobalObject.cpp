// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"

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
void CGlobalObject::SetTransformation(CObjectTransformation const& transformation)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set transformation on the global object!");
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set an environment on the global object!");
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	auto const pParameter = static_cast<CParameter const*>(pIParameter);

	if (pParameter != nullptr)
	{
		EParameterType const type = pParameter->GetType();

		switch (type)
		{
		case EParameterType::AisacControl:
			{
				for (auto const pObject : m_objects)
				{
					if (pObject != this)
					{
						pObject->SetParameter(pParameter, value);
					}
					else
					{
						criAtomExPlayer_SetAisacControlByName(m_pPlayer, pParameter->GetName(), static_cast<CriFloat32>(pParameter->GetMultiplier() * value + pParameter->GetValueShift()));
						criAtomExPlayer_UpdateAll(m_pPlayer);
					}
				}
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
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid Parameter pointer passed to the Adx2 implementation of SetParameter");
	}
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	auto const pSwitchState = static_cast<CSwitchState const*>(pISwitchState);

	if (pSwitchState != nullptr)
	{
		ESwitchType const type = pSwitchState->GetType();

		switch (type)
		{
		case ESwitchType::Selector:
		case ESwitchType::AisacControl:
			{
				for (auto const pObject : m_objects)
				{
					if (pObject != this)
					{
						pObject->SetSwitchState(pISwitchState);
					}
					else
					{
						if (type == ESwitchType::Selector)
						{
							criAtomExPlayer_SetSelectorLabel(m_pPlayer, pSwitchState->GetName(), pSwitchState->GetLabelName());
							criAtomExPlayer_UpdateAll(m_pPlayer);
						}
						else
						{
							criAtomExPlayer_SetAisacControlByName(m_pPlayer, pSwitchState->GetName(), pSwitchState->GetValue());
							criAtomExPlayer_UpdateAll(m_pPlayer);
						}
					}
				}
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
		Cry::Audio::Log(ELogType::Error, "Invalid switch pointer passed to the Adx2 implementation of SetSwitchState");
	}
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion and obstruction values on the global object!");
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusionType(EOcclusionType const occlusionType)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion type on the global object!");
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
