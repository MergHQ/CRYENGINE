// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "VersionControlFileStatus.h"
#include "VersionControlFileConflictStatus.h"
#include "VersionControlError.h"
#include <unordered_map>

struct SPreferencePage;
class CVersionControlCache;

//! Interface for VCS backend implementation.
struct EDITOR_COMMON_API IVersionControlAdapter
{
	using FileStatusesMap = std::unordered_map<string, std::shared_ptr<CVersionControlFileStatus>, stl::hash_strcmp<string>, stl::hash_strcmp<string>>;

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

	virtual SVersionControlError UpdateStatus(const std::vector<string>& filePaths, const std::vector<string>& folders = {}) = 0;

	virtual SVersionControlError UpdateStatus() = 0;

	virtual SVersionControlError GetLatest(const std::vector<string>& files, const std::vector<string>& folders, bool force) = 0;

	virtual SVersionControlError SubmitFiles(const std::vector<string>& filePaths, const string& message = "") = 0;

	virtual SVersionControlError ResolveConflicts(const std::vector<SVersionControlFileConflictStatus>& conflictStatuses) = 0;

	virtual SVersionControlError AddFiles(const std::vector<string>& filePaths) = 0;

	virtual SVersionControlError EditFiles(const std::vector<string>& filePaths) = 0;

	virtual SVersionControlError DeleteFiles(const std::vector<string>& filePaths) = 0;

	virtual SVersionControlError Revert(const std::vector<string>& files, const std::vector<string>& folders) = 0;

	virtual SVersionControlError RevertUnchanged(const std::vector<string>& files, const std::vector<string>& folders) = 0;

	virtual SVersionControlError CheckSettings() = 0;

	virtual SPreferencePage* GetPreferences() = 0;

	virtual bool             IsOnline() const = 0;

	CCrySignal<void()>       signalOnlineChanged;

protected:
	std::shared_ptr<CVersionControlCache> GetCache();
public:
};

