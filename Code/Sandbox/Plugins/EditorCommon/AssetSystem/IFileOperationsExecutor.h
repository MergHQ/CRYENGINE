// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#pragma once

#include "IFilesGroupProvider.h"

class EDITOR_COMMON_API IFileOperationsExecutor
{
public:
	virtual ~IFileOperationsExecutor() {}

	//! Physically removed files from file system.
	//! This method is run on a separate thread.
	void Delete(std::vector<std::unique_ptr<IFilesGroupProvider>> fileGroups);
	void Delete(const std::vector<string>& files);

protected:
	void virtual DoDelete(std::vector<std::unique_ptr<IFilesGroupProvider>> fileGroups) = 0;
	void virtual DoDelete(const std::vector<string>& files) = 0;
};
