// Copyright 2001-2019 Crytek GmbH. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "IFilesGroupController.h"

class EDITOR_COMMON_API IFileOperationsExecutor
{
public:
	virtual ~IFileOperationsExecutor() {}

	//! Physically removed file groups from file system.
	//! The method is executed on another thread (not main).
	//! If it is being called not from main thread then the execution will continue on the current
	//! thread unless forceAsync flag is set to true.
	void AsyncDelete(std::vector<std::unique_ptr<IFilesGroupController>> fileGroups, std::function<void(void)> callback = nullptr, bool forceAsync = false);

	//! Physically removed files from file system.
	//! The method is executed on another thread (not main).
	//! If it is being called not from main thread then the execution will continue on the current
	//! thread unless forceAsync flag is set to true.
	void AsyncDelete(const std::vector<string>& files, std::function<void(void)> callback = nullptr, bool forceAsync = false);

	//! Physically removed file groups from file system.
	//! This method is executed on the current thread which should not be main thread.
	//! If it is being called not from main thread then the execution will continue on the current
	//! thread unless forceAsync flag is set to true.
	void Delete(std::vector<std::unique_ptr<IFilesGroupController>> fileGroups, std::function<void(void)> callback = nullptr, bool forceAsync = false);

	//! Physically removed files from file system.
	//! This method is executed on the current thread which should not be main thread.
	//! If it is being called not from main thread then the execution will continue on the current
	//! thread unless forceAsync flag is set to true.
	void Delete(const std::vector<string>& files, std::function<void(void)> callback = nullptr, bool forceAsync = false);

protected:
	void virtual DoDelete(std::vector<std::unique_ptr<IFilesGroupController>> fileGroups, std::function<void(void)>) = 0;
	void virtual DoDelete(const std::vector<string>& files, std::function<void(void)>) = 0;
};
