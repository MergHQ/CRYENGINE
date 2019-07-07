// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlCache.h"
#include "VersionControlFileStatusUpdate.h"
#include <IEditor.h>
#include <CrySystem/ISystem.h>

void CVersionControlCache::UpdateFiles(const std::vector<CVersionControlFileStatusUpdate>& newFiles, bool shouldClear /*= false*/)
{
	if (newFiles.empty())
	{
		m_lastUpdateList.clear();
		return;
	}
	bool hasChanges = false;
	{
		std::lock_guard<std::mutex> lock(m_fileStatusesMutex);
		if (shouldClear)
		{
			m_fileStatuses.clear();
			hasChanges = true;
		}
		for (const auto& fileStatusUpdate : newFiles)
		{
			auto it = m_fileStatuses.find(fileStatusUpdate.GetFileName());
			if (it != m_fileStatuses.end())
			{
				const CVersionControlFileStatus& fs = *it->second;
				const auto oldState = fs.GetState();
				fileStatusUpdate.Apply(*it->second);
				hasChanges |= oldState != fs.GetState();
			}
			else
			{
				auto fs = std::make_shared<CVersionControlFileStatus>(fileStatusUpdate.GetFileName());
				fileStatusUpdate.Apply(*fs);
				m_fileStatuses.emplace(fileStatusUpdate.GetFileName(), std::move(fs));
				hasChanges = true;
			}
		}
	}
	m_lastUpdateList = newFiles;
	if (hasChanges)
	{
		SendUpdateSignal();
	}
}

void CVersionControlCache::Clear()
{
	{
		std::lock_guard<std::mutex> lock(m_fileStatusesMutex);
		m_fileStatuses.clear();
	}
	{
		std::lock_guard<std::mutex> lock(m_fileContentsMutex);
		m_filesContents.clear();
	}
	m_lastUpdateList.clear();
	SendUpdateSignal();
}

void CVersionControlCache::SaveFilesContent(const string& file, const string& filesContent)
{
	std::lock_guard<std::mutex> lock(m_fileContentsMutex);
	m_filesContents[file] = filesContent;
}

string CVersionControlCache::RemoveFilesContent(const string& file)
{
	std::lock_guard<std::mutex> lock(m_fileContentsMutex);
	auto it = m_filesContents.find(file);
	if (it != m_filesContents.end())
	{
		string filesContent = std::move(it->second);
		m_filesContents.erase(it);
		return filesContent;
	}
	return "";
}

const string& CVersionControlCache::GetFilesContent(const string& file) const
{
	static const string emptyString;
	std::lock_guard<std::mutex> lock(m_fileContentsMutex);
	const auto& it = m_filesContents.find(file);
	if (it != m_filesContents.cend())
	{
		return it->second;
	}
	return emptyString;
}

void CVersionControlCache::GetFileStatuses(std::function<bool(const CVersionControlFileStatus&)> filter
	, std::vector<std::shared_ptr<const CVersionControlFileStatus>>& result) const
{
	std::lock_guard<std::mutex> lock(m_fileStatusesMutex);
	for (const auto& fsItem : m_fileStatuses)
	{
		if (filter(*fsItem.second))
		{
			result.push_back(fsItem.second);
		}
	}
}

std::shared_ptr<const CVersionControlFileStatus> CVersionControlCache::GetFileStatus(const string& filePath)
{
	std::lock_guard<std::mutex> lock(m_fileStatusesMutex);
	auto it = m_fileStatuses.find(filePath);
	if (it != m_fileStatuses.end())
	{
		return it->second;
	}

	return nullptr;
}

void CVersionControlCache::SendUpdateSignal()
{
	if (CryGetCurrentThreadId() == gEnv->mMainThreadId)
	{
		m_signalUpdate();
	}
	else
	{
		GetIEditor()->PostOnMainThread([this]()
		{
			m_signalUpdate();
		});
	}
}
