// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"
#include "Common.h"
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
CGlobalObject::CGlobalObject()
{
	CRY_ASSERT_MESSAGE(g_pObject == nullptr, "g_pObject is not nullptr during %s", __FUNCTION__);
	g_pObject = this;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	m_name = "Global Object";
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
CGlobalObject::~CGlobalObject()
{
	g_pObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusion(float const occlusion)
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