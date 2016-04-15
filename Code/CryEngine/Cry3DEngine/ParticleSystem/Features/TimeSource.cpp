// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TimeSource.h"

CRY_PFX2_DBG

volatile bool gPfx2TimeSource = false;

namespace pfx2
{

SERIALIZATION_ENUM_IMPLEMENT(ETimeSource);
SERIALIZATION_ENUM_IMPLEMENT(ETimeSourceField);

}
