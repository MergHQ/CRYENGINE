// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Environment.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
CEnvironment::CEnvironment(EEnvironmentType const type_, AkAuxBusID const busId_)
	: type(type_)
	, busId(busId_)
{
	CRY_ASSERT(type_ == EEnvironmentType::AuxBus);
}

//////////////////////////////////////////////////////////////////////////
CEnvironment::CEnvironment(
	EEnvironmentType const type_,
	AkRtpcID const rtpcId_,
	float const multiplier_,
	float const shift_)
	: type(type_)
	, rtpcId(rtpcId_)
	, multiplier(multiplier_)
	, shift(shift_)
{
	CRY_ASSERT(type_ == EEnvironmentType::Rtpc);
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
