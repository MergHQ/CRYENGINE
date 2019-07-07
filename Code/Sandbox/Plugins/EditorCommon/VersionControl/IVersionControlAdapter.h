// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "VersionControlFileStatus.h"
#include "VersionControlFileConflictStatus.h"
#include "VersionControlError.h"
#include <CrySandbox/CrySignal.h>
#include <CryCore/StlUtils.h>
#include <unordered_map>

struct SPreferencePage;
class CVersionControlCache;

//! Interface for VCS backend implementation.
struct EDITOR_COMMON_API IVersionControlAdapter
{
	using FileStatusesMap = std::unordered_map<string, std::shared_ptr<CVersionControlFileStatus>, stl::hash_stricmp<string>, stl::hash_stricmp<string>>;

	enum class EConflictResolutionStrategy
	{
		Add = 0,
		Delete,
		KeepOurs,
		TakeTheir,
		None,
		Count
	};

	virtual ~IVersionControlAdapter();

	//! Updates version control status of given files and folders.
	virtual SVersionControlError UpdateStatus(const std::vector<string>& files, const std::vector<string>& folders = {}) = 0;

	//! Updates version control status of all tracked files.
	virtual SVersionControlError UpdateStatus() = 0;

	//! Downloads the last revision of given files and folders
	//! \param fileExtensions If given only files with matching extension will be downloaded. Can be applied only to folders.
	//! \param force Specifies if update to date files need to be forced to update.
	virtual SVersionControlError GetLatest(const std::vector<string>& files, const std::vector<string>& folders, const std::vector<string>& fileExtensions, bool force) = 0;

	//! Submits files to repository.
	//! \param message Description for the current commit.
	virtual SVersionControlError SubmitFiles(const std::vector<string>& files, const string& message = "") = 0;

	//! Resolves conflicts.
	virtual SVersionControlError ResolveConflicts(const std::vector<SVersionControlFileConflictStatus>& conflictStatuses) = 0;

	//! Marks given files for add in version control system.
	virtual SVersionControlError AddFiles(const std::vector<string>& files) = 0;

	//! Marks given files as being edited (or checks out).
	virtual SVersionControlError EditFiles(const std::vector<string>& files) = 0;

	//! Marks given files for delete in version control system.
	virtual SVersionControlError DeleteFiles(const std::vector<string>& files) = 0;

	//! Reverts all local changes to the files or folders.
	virtual SVersionControlError Revert(const std::vector<string>& files, const std::vector<string>& folders) = 0;

	//! Clear files' and folders' local state if they are not changed.
	//! \param clearIfUnchanged Specifies if the state needs be cleared only for files and folders that didn't change.
	virtual SVersionControlError ClearLocalState(const std::vector<string>& files, const std::vector<string>& folders, bool clearIfUnchanged) = 0;

	//! Retrieves file's textual content from repository.
	virtual SVersionControlError RetrieveFilesContent(const string& file) = 0;

	//! Removes files from the files system in VCS-friendly way.
	virtual SVersionControlError RemoveFilesLocally(const std::vector<string>& files) = 0;

	//! Check is current version control settings are valid.
	virtual SVersionControlError CheckSettings() = 0;

	//! Returns preference page specific for the current version control system.
	virtual SPreferencePage* GetPreferences() = 0;

	//! Specifies and version control system is current connected.
	virtual bool             IsOnline() const = 0;

	//! Signal that is fired any time online state of version control system changes.
	CCrySignal<void()>       signalOnlineChanged;

protected:
	std::shared_ptr<CVersionControlCache> GetCache();
};
