// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IVersionControlAdapter.h"
#include "EditorCommonAPI.h"
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

	size_t GetSize() const { return m_fileStatuses.size(); }

	void GetFileStatuses(std::function<bool(const CVersionControlFileStatus&)>
		, std::vector<std::shared_ptr<const CVersionControlFileStatus>>&) const; 

	std::shared_ptr<const CVersionControlFileStatus> GetFileStatus(const string& filePath);

	CCrySignal<void()> m_signalUpdate;

private:
	void SendUpdateSignal();

	FileStatusesMap m_fileStatuses;

	mutable std::mutex m_dataMutex;
};
