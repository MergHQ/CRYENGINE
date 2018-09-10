// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
CParameter::CParameter(AkRtpcID const id_, float const mult_, float const shift_)
	: mult(mult_)
	, shift(shift_)
	, id(id_)
{
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
