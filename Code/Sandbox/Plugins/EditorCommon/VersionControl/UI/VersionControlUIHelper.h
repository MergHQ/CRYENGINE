// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <QIcon>

class CItemModelAttribute;

namespace VersionControlUIHelper
{
EDITOR_COMMON_API QIcon                GetIconFromStatus(int status);

EDITOR_COMMON_API int                  GetVCSStatusRole();

EDITOR_COMMON_API CItemModelAttribute* GetVCSStatusAttribute();
}