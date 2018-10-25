// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "VersionControlError.h"

class CVersionControl;
class CVersionControlTaskManager;

//! This class is responsible for containing the results of execution of a VCS task.
class EDITOR_COMMON_API CVersionControlResult
{
public:
	bool IsSuccess() const { return m_error.type == EVersionControlError::None; }

	SVersionControlError GetError() const { return m_error; }

private:
	void SetError(SVersionControlError error) { m_error = error; }

	SVersionControlError m_error;

	friend CVersionControl;
	friend CVersionControlTaskManager;
};
