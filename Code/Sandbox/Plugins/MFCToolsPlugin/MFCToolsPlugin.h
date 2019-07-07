// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "MFCToolsDefines.h"

//As this is not initialized as a real plugin, it needs to behave similarly to EditorCommon and be initialized manually at the right time
namespace MFCToolsPlugin
{
	//!Sets editor and gEnv, needs both to be initialized
	MFC_TOOLS_PLUGIN_API void SetEditor(IEditor* editor);

	MFC_TOOLS_PLUGIN_API void Initialize();
	MFC_TOOLS_PLUGIN_API void Deinitialize();
}
