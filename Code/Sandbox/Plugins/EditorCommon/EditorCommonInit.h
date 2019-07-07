// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"

struct IEditor;

namespace EditorCommon
{

//! Preferably be called after IEditor is fully initialized
void EDITOR_COMMON_API SetIEditor(IEditor* editor);
void EDITOR_COMMON_API Initialize();
void EDITOR_COMMON_API Deinitialize();
}
