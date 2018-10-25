// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "AssetSystem/IFileOperationsExecutor.h"
#include "CryString/CryString.h"
#include <vector>

class EDITOR_COMMON_API CVersionControlFileOperationsExecutor : public IFileOperationsExecutor
{
public:
	virtual void DoDelete(std::vector<std::unique_ptr<IFilesGroupProvider>> fileGroups) override;
	virtual void DoDelete(const std::vector<string>& files) override;
};
