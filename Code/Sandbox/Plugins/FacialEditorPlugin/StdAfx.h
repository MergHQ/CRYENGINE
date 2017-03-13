// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Platform/platform.h>

#define CRY_USE_XT
#include <CryCore/Platform/CryAtlMfc.h>

#include <CryAnimation/ICryAnimation.h>
#include "EditorCommon.h"

#include "PluginAPI.h"

#include "Resource.h"

#include "Util/Variable.h"

#define QT_STRINGANIZE2(x) # x
#define QT_STRINGANIZE1(x) QT_STRINGANIZE2(x)
#define QT_TODO(y)         __pragma(message(__FILE__ "(" QT_STRINGANIZE1(__LINE__) ") : " "[QT] TODO >>> " QT_STRINGANIZE2(y)))