// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IVersionControlAdapter.h"
#include "VersionControlFileStatusUpdate.h"
#include "EditorCommonAPI.h"
#include <CrySandbox/CrySignal.h>
#include <unordered_map>
#include <mutex>

class CVersionControlFileStatusUpdate;

using FileStatusesMap = IVersionControlAdapter::FileStatusesMap;

//! The cache of version control system that stores all information about tracked files.
class EDITOR_COMMON_API CVersionControlCache
{
public:
	//! Updates statuses of files under VCS.
	//! \see CVersionControlFileStatusUpdate. 
	void UpdateFiles(const std::vector<CVersionControlFileStatusUpdate>&, bool shouldClear = false);

	//! Clears cache.
	void Clear();

	//! Saves the content a given file in the cache.
	void SaveFilesContent(const string& file, const string& filesContent);

	//! Returns the content for a given file and removes it from cache.
	string RemoveFilesContent(const string& file);

	//! Returns the content of a given file.
	//! @return empty string if not found.
	const string& GetFilesContent(const string& file) const;

	//! Returns the number of files' statuses in cache.
	size_t GetSize() const { return m_fileStatuses.size(); }

	//! Filters file statuses.
	//! @filter Filter function.
	//! @result Resulting filtered list of files' statuses.
	void   GetFileStatuses(std::function<bool(const CVersionControlFileStatus&)> filter
		, std::vector<std::shared_ptr<const CVersionControlFileStatus>>& result) const; 

	//! Returns version control status of given file.
	std::shared_ptr<const CVersionControlFileStatus> GetFileStatus(const string& filePath);

	CCrySignal<void()> m_signalUpdate;

private:
	void SendUpdateSignal();

	FileStatusesMap m_fileStatuses;

	//! Holds list of last updates to file statuses. So UpdateFiles() needs to be called only once during a task execution.
	std::vector<CVersionControlFileStatusUpdate> m_lastUpdateList;

	std::unordered_map<string, string, stl::hash_strcmp<string>, stl::hash_strcmp<string>> m_filesContents;

	mutable std::mutex m_fileStatusesMutex;
	mutable std::mutex m_fileContentsMutex;

	//! task manager manipulates only with the list of last updates to assign it to the result of completed task.
	friend class CVersionControlTaskManager;
};
