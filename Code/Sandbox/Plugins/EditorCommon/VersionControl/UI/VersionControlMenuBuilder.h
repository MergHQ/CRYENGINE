// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"

//! This class is responsible for building context menu for version control system actions.
class EDITOR_COMMON_API CVersionControlMenuBuilder 
{
public:
	//! Activates the builder by starting listening to context menu requests.
	void Activate();

	//! Deactivates the builder by stopping listening to context menu requests.
	void Deactivate();
};