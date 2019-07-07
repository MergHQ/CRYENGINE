// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "VersionControlError.h"
#include "VersionControlFileStatusUpdate.h"

//! This class is responsible for containing the results of execution of a VCS task.
class EDITOR_COMMON_API CVersionControlResult
{
public:
	bool IsSuccess() const { return m_error.type == EVersionControlError::None; }

	SVersionControlError GetError() const { return m_error; }

	const std::vector<CVersionControlFileStatusUpdate>& GetStatusChanges() const { return m_statusChanges; }

private:
	void SetError(SVersionControlError error) { m_error = error; }

	SVersionControlError m_error;
	std::vector<CVersionControlFileStatusUpdate> m_statusChanges;

	friend class CVersionControl;
	friend class CVersionControlTaskManager;
};
