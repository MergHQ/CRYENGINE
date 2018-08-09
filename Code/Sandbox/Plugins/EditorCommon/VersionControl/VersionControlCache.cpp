// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlCache.h"
#include "VersionControlFileStatusUpdate.h"

void CVersionControlCache::UpdateFiles(const std::vector<CVersionControlFileStatusUpdate>& newFiles, bool shouldClear /*= false*/)
{
	if (newFiles.empty())
	{
		return;
	}
	{
		std::lock_guard<std::mutex> lock(m_dataMutex);
		if (shouldClear)
		{
			m_fileStatuses.clear();
		}
		for (const auto& fileStatus : newFiles)
		{
			auto it = m_fileStatuses.find(fileStatus.GetFileName());
			if (it != m_fileStatuses.end())
			{
				fileStatus.Apply(*it->second);
			}
			else
			{
				auto fs = std::make_shared<CVersionControlFileStatus>(fileStatus.GetFileName());
				fileStatus.Apply(*fs);
				m_fileStatuses.emplace(fileStatus.GetFileName(), std::move(fs));
			}
		}
	}
	SendUpdateSignal();
}

void CVersionControlCache::Clear()
{
	{
		std::lock_guard<std::mutex> lock(m_dataMutex);
		m_fileStatuses.clear();
	}
	SendUpdateSignal();
}

void CVersionControlCache::GetFileStatuses(std::function<bool(const CVersionControlFileStatus&)> filter
	, std::vector<std::shared_ptr<const CVersionControlFileStatus>>& result) const
{
	std::lock_guard<std::mutex> lock(m_dataMutex);
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
	std::lock_guard<std::mutex> lock(m_dataMutex);
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
