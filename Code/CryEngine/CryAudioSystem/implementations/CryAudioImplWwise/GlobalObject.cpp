// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetTransformation(CTransformation const& transformation)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set transformation on the global object!");
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusion(float const occlusion)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion and obstruction values on the global object!");
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusionType(EOcclusionType const occlusionType)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion type on the global object!");
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}
}      // namespace Wwise
}      // namespace Impl
}      // namespace CryAudio
