// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"

#include "Common.h"
#include "Environment.h"

#include <Logger.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
CGlobalObject::CGlobalObject()
{
	CRY_ASSERT_MESSAGE(g_pObject == nullptr, "g_pObject is not nullptr during %s", __FUNCTION__);
	g_pObject = this;
}

//////////////////////////////////////////////////////////////////////////
CGlobalObject::~CGlobalObject()
{
	g_pObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetTransformation(CTransformation const& transformation)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set transformation on the global object!");
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set an environment on the global object!");
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
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
