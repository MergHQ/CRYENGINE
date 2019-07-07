// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "IVersionControlAdapter.h"
#include "VersionControlCache.h"
#include "VersionControlFileConflictStatus.h"
#include "VersionControlTaskManager.h"
#include <CrySandbox/CrySignal.h>
#include <memory>

struct SPreferencePage;

namespace Private_VersionControl
{
	struct SVersionControlPreferences;
}

//! This class is main entry point for communicating with version control system.
//! All methods here that return std::shared_ptr<CVersionControlTask> are executed asynchronously. 
//! All of them accept a parameter that allow to specify if the call should be blocking or not. 
//! Blocking call can not be made on main thread.
//! For non-blocking call a callback can be passed to these methods that would be called upon task completion.
class EDITOR_COMMON_API CVersionControl
{
public:
	using Callback = CVersionControlTaskManager::Callback;

	static CVersionControl& GetInstance()
	{
		static CVersionControl instance;
		return instance;
	}

	//! Specifies if version control system is available, e.g. selected in the settings.
	static bool IsAvailable();

	CVersionControl(const CVersionControl&) = delete;
	CVersionControl& operator=(const CVersionControl&) = delete;
	CVersionControl(CVersionControl&&) = delete;
	CVersionControl& operator=(CVersionControl&&) = delete;

	//! Returns the version control status of a given file.
	std::shared_ptr<const CVersionControlFileStatus> GetFileStatus(const string& filePath) const { return m_pCache->GetFileStatus(filePath); }

	//! Filters version control file statuses.
	void GetFileStatuses(std::function<bool(const CVersionControlFileStatus&)> f, std::vector<std::shared_ptr<const CVersionControlFileStatus>>& result) const { m_pCache->GetFileStatuses(f, result); }

	//! Returns a given file's textual content and removes it from cache.
	string RemoveFilesContent(const string& file) { return m_pCache->RemoveFilesContent(file); }

	//! Returns a given file's textual content.
	const string& GetFilesContent(const string& file) const { return m_pCache->GetFilesContent(file); }

	//! Returns the signal that is sent when the file's statuses data is changed.
	CCrySignal<void()>& GetUpdateSignal() { return m_pCache->m_signalUpdate; }

	//! Returns the number of files' statuses in the cache.
	size_t GetFilesNum() const { return m_pCache->GetSize(); }

	//! Updates version control status given files or folders.
	std::shared_ptr<CVersionControlTask> UpdateFileStatus(std::vector<string> filePaths, std::vector<string> folders = {}, bool isBlocking = false, Callback callback = {});

	//! Updates version control status of all files tracked by VCS.
	std::shared_ptr<CVersionControlTask> UpdateStatus(bool isBlocking = false, Callback callback = {});

	//! Downloads (or synchronizes to) the latest version of gives files and folders.
	//! \param fileExtensions If given only files with matching extension will be downloaded. Can be applied only to folders.
	//! \param force Specifies if update to date files need to be forced to update.
	std::shared_ptr<CVersionControlTask> GetLatest(std::vector<string> files, std::vector<string> folders, std::vector<string> fileExtensions = {}, bool force = false, bool isBlocking = false, Callback callback = {});

	//! Submits files to the remote repository.
	//! \param message The message to be associated with the commit.
	std::shared_ptr<CVersionControlTask> SubmitFiles(std::vector<string> filePaths, const string& message, bool isBlocking = false, Callback callback = {});

	//! Resolves conflicts.
	std::shared_ptr<CVersionControlTask> ResolveConflicts(std::vector<SVersionControlFileConflictStatus> resolutions, bool isBlocking = false, Callback callback = {});

	//! Adds new files to be tracked by VCS.
	std::shared_ptr<CVersionControlTask> AddFiles(std::vector<string> filePath, bool isBlocking = false, Callback callback = {});
	
	//! Checks out (or marks for edit) files in VCS.
	std::shared_ptr<CVersionControlTask> EditFiles(std::vector<string> filePath, bool isBlocking = false, Callback callback = {});

	//! Deletes files from VCS so that they won't be tracked anymore.
	std::shared_ptr<CVersionControlTask> DeleteFiles(std::vector<string> filePath, bool isBlocking = false, Callback callback = {});

	//! Reverts any local changes made to given files and folders.
	std::shared_ptr<CVersionControlTask> Revert(std::vector<string> filePath, std::vector<string> folders = {}, bool isBlocking = false, Callback callback = {});

	//! Clears local state of the given files and folders.
	//! \param clearIfUnchanged Specifies if local states need to be cleared only if they are unchanged.
	std::shared_ptr<CVersionControlTask> ClearLocalState(std::vector<string> filePath, std::vector<string> folders, bool clearIfUnchanged, bool isBlocking = false, Callback callback = {});

	//! Retrieves textual content from repository of a given file.
	std::shared_ptr<CVersionControlTask> RetrieveFilesContent(const string& file, bool isBlocking = false, CVersionControl::Callback callback = {});

	//! Removes local version of the file from the files system. 
	//! It needs to be done in VCS-friendly way in order to retrieve it back when needed.
	std::shared_ptr<CVersionControlTask> RemoveFilesLocally(std::vector<string> files, bool isBlocking = false, Callback = {});

	//! Checks if the current settings are correct.
	std::shared_ptr<CVersionControlTask> CheckSettings(bool isBlocking = false, Callback callback = {});

	//! Returns preferences page to be serialized.
	SPreferencePage* GetPreferences() { return m_pAdapter ? m_pAdapter->GetPreferences() : nullptr; }

	//! Specifies if currently active version control system is online, e.g. connected to the server.
	bool             IsOnline() const { return m_pAdapter ? m_pAdapter->IsOnline() : false; }

	CCrySignal<void()> signalOnlineChanged;

private:
	CVersionControl()
		: m_pCache(std::make_shared<CVersionControlCache>())
		, m_taskManager(m_pCache.get())
	{}

	bool UpdateAdapter();

	void OnAdapterOnlineChanged();

	std::shared_ptr<CVersionControlCache> GetCache() { return m_pCache; }

	std::shared_ptr<IVersionControlAdapter> m_pAdapter;
	std::shared_ptr<CVersionControlCache>   m_pCache;
	CVersionControlTaskManager              m_taskManager;

	friend struct IVersionControlAdapter;
	friend struct Private_VersionControl::SVersionControlPreferences;
};
