// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "AssetSystem/IFileOperationsExecutor.h"
#include "CryString/CryString.h"
#include <vector>

class EDITOR_COMMON_API CVersionControlFileOperationsExecutor : public IFileOperationsExecutor
{
public:
	virtual void DoDelete(std::vector<std::unique_ptr<IFilesGroupController>> fileGroups, std::function<void(void)>) override;
	virtual void DoDelete(const std::vector<string>& files, std::function<void(void)>) override;
};
