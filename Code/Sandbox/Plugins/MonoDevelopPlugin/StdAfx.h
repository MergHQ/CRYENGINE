// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// CryEngine headers.

#include <CryCore/Project/CryModuleDefs.h>
#include <CryCore/Platform/platform.h>

#define CRY_USE_XT
#include <CryCore/Platform/CryAtlMfc.h>

// STL headers.

#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

// Additional CryEngine headers.

#include <SandboxAPI.h>
#include <CrySystem/ISystem.h>
#include <CryCore/functor.h>

#include "EditorCommon.h"
#include "Util/EditorUtils.h"
#include "IEditor.h"
#include "Util/PathUtil.h"
#include "Util/SmartPtr.h"
#include "Util/Variable.h"

IEditor*  GetIEditor();
HINSTANCE GetHInstance();

