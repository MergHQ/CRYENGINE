// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"
#include "Environment.h"
#include "Parameter.h"
#include "SwitchState.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CGlobalObject::CGlobalObject(Objects const& objects)
	: m_objects(objects)
{
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set an environment on the global object!");
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	CBaseParameter const* const pParameter = static_cast<CBaseParameter const* const>(pIParameter);

	if (pParameter != nullptr)
	{
		for (auto const pObject : m_objects)
		{
			if (pObject != this)
			{
				pObject->SetParameter(pParameter, value);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid parameter pointer passed to the Fmod implementation of SetParameter");
	}
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	CBaseSwitchState const* const pSwitchState = static_cast<CBaseSwitchState const* const>(pISwitchState);

	if (pSwitchState != nullptr)
	{
		for (auto const pObject : m_objects)
		{
			if (pObject != this)
			{
				pObject->SetSwitchState(pISwitchState);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid switch pointer passed to the Fmod implementation of SetSwitchState");
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

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
	if (enable)
	{
		char const* szType = nullptr;

		switch (type)
		{
		case EObjectFunctionality::TrackAbsoluteVelocity:
			{
				szType = "absolute velocity tracking";

				break;
			}
		case EObjectFunctionality::TrackRelativeVelocity:
			{
				szType = "relative velocity tracking";

				break;
			}
		default:
			{
				szType = "an undefined functionality";

				break;
			}
		}

		Cry::Audio::Log(ELogType::Error, "Trying to enable %s on the global object!", szType);
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio