// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include "EditorCommonAPI.h"

struct EDITOR_COMMON_API ViewportInteraction
{
	enum Key
	{
		eKey_Forward,
		eKey_Backward,
		eKey_Left,
		eKey_Right
	};

	static bool CheckPolledKey(Key key);
};
