// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IVersionControlAdapter.h"
#include "VersionControlCache.h"
#include "VersionControlFileConflictStatus.h"
#include "VersionControlTaskManager.h"
#include <memory>

struct SPreferencePage;

//! This class is main entry point for communicating with version control system.
//! All methods here that return std::shared_ptr<const CVersionControlResult> are executed asynchronously. 
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

	static bool IsAvailable();

	CVersionControl(const CVersionControl&) = delete;
	CVersionControl& operator=(const CVersionControl&) = delete;
	CVersionControl(CVersionControl&&) = delete;
	CVersionControl& operator=(CVersionControl&&) = delete;

	std::shared_ptr<const CVersionControlFileStatus> GetFileStatus(const string& filePath) const { return m_pCache->GetFileStatus(filePath); }

	void GetFileStatuses(std::function<bool(const CVersionControlFileStatus&)> f, std::vector<std::shared_ptr<const CVersionControlFileStatus>>& result) const { m_pCache->GetFileStatuses(f, result); }

	CCrySignal<void()>& GetUpdateSignal() { return m_pCache->m_signalUpdate; }

	size_t GetFilesNum() const { return m_pCache->GetSize(); }

	std::shared_ptr<const CVersionControlResult> UpdateFileStatus(std::vector<string> filePaths, std::vector<string> folders = {}, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> UpdateStatus(bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> GetLatest(std::vector<string> files, std::vector<string> folders, bool force = false, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> SubmitFiles(std::vector<string> filePaths, const string& message, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> ResolveConflicts(std::vector<SVersionControlFileConflictStatus> resolutions, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> AddFiles(std::vector<string> filePath, bool isBlocking = false, Callback callback = {});
	
	std::shared_ptr<const CVersionControlResult> EditFiles(std::vector<string> filePath, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> DeleteFiles(std::vector<string> filePath, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> Revert(std::vector<string> filePath, std::vector<string> folders = {}, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> RevertUnchanged(std::vector<string> filePath, std::vector<string> folders, bool isBlocking = false, Callback callback = {});

	std::shared_ptr<const CVersionControlResult> CheckSettings(bool isBlocking = false, Callback callback = {});

	SPreferencePage* GetPreferences() { return m_pAdapter ? m_pAdapter->GetPreferences() : nullptr; }

	bool             IsOnline() const { return m_pAdapter ? m_pAdapter->IsOnline() : false; }

	CCrySignal<void()> signalOnlineChanged;

private:
	CVersionControl()
		: m_pCache(std::make_shared<CVersionControlCache>())
	{}

	bool UpdateAdapter();

	void OnAdapterOnlineChanged();

	std::shared_ptr<CVersionControlCache> GetCache() { return m_pCache; }

	std::shared_ptr<IVersionControlAdapter> m_pAdapter;
	std::shared_ptr<CVersionControlCache>   m_pCache;
	CVersionControlTaskManager              m_taskManager;

	friend struct SVersionControlPreferences;
	friend struct IVersionControlAdapter;
};
