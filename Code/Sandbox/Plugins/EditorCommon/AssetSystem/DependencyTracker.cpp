// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DependencyTracker.h"
#include "AssetManager.h"
#include "PathUtils.h"
#include "Util/EditorUtils.h"

namespace Private_DependencyTracker
{

struct SLessKey
{
	bool operator()(const std::pair<string, SAssetDependencyInfo>& a, const string& b)
	{
		return stricmp(a.first.c_str(), b.c_str()) < 0;
	}

	bool operator()(const string& a, const std::pair<string, SAssetDependencyInfo>& b)
	{
		return stricmp(a.c_str(), b.first.c_str()) < 0;
	}
};

struct SLessValue
{
	bool operator()(const std::pair<string, SAssetDependencyInfo>& a, const string& b)
	{
		return stricmp(a.second.path.c_str(), b.c_str()) < 0;
	}

	bool operator()(const string& a, const std::pair<string, SAssetDependencyInfo>& b)
	{
		return stricmp(a.c_str(), b.second.path.c_str()) < 0;
	}
};

}


CDependencyTracker::CDependencyTracker()
{
	CAssetManager* pAssetManager = CAssetManager::GetInstance();

	pAssetManager->signalAfterAssetsInserted.Connect([this](const std::vector<CAsset*>&)
	{
		CreateIndex();
	}, (uintptr_t)this);

	pAssetManager->signalAssetChanged.Connect([this](CAsset&)
	{
		CreateIndex();
	}, (uintptr_t)this);

	pAssetManager->signalAfterAssetsRemoved.Connect([this]()
	{
		CreateIndex();
	}, (uintptr_t)this);
}

CDependencyTracker::~CDependencyTracker()
{
	CAssetManager* pAssetManager = CAssetManager::GetInstance();
	pAssetManager->signalAfterAssetsInserted.DisconnectById((uintptr_t)this);
	pAssetManager->signalAssetChanged.DisconnectById((uintptr_t)this);
	pAssetManager->signalAfterAssetsRemoved.DisconnectById((uintptr_t)this);
}

std::vector<SAssetDependencyInfo> CDependencyTracker::GetReverseDependencies(const char* szAssetPath) const
{
	const auto range = GetRange(szAssetPath);

	std::vector<SAssetDependencyInfo> dependencies;
	dependencies.reserve(3); // just an empirical expectation.

	std::transform(range.first, range.second, std::back_inserter(dependencies), [](const auto& x)
	{
		return x.second;
	});

	return dependencies;
}

std::pair<bool, int>  CDependencyTracker::IsAssetUsedBy(const char* szAssetPath, const char* szAnotherAssetPath) const
{
	using namespace Private_DependencyTracker;

	const auto range = GetRange(szAssetPath);
	if (range.first == range.second)
	{
		return { false, 0 };
	}

	const string value = PathUtil::ToUnixPath(szAnotherAssetPath);
	const auto lowerBound = std::lower_bound(range.first, range.second, value, SLessValue());
	if (lowerBound != range.second && (*lowerBound).second.path.CompareNoCase(value) == 0)
	{
		return { true, (*lowerBound).second.usageCount };
	}

	return { false, 0 };
}

void CDependencyTracker::CreateIndex()
{
	// Do nothing if the future has already referred to the deffered request.
	if (m_future.valid() && m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
	{
		return;
	}

	m_future = std::async(std::launch::deferred, [this]()
	{
		m_index.clear();
		CAssetManager* pAssetManager = CAssetManager::GetInstance();
		pAssetManager->ForeachAsset([this](CAsset* pAsset)
		{
			MAKE_SURE(pAsset->GetFilesCount(), return);

			const std::vector<SAssetDependencyInfo>& dependencies = pAsset->GetDependencies();
			for (const auto& item : dependencies)
			{
				m_index.emplace_back(item.path, SAssetDependencyInfo( pAsset->GetFile(0), item.usageCount ));
			}
		});

		std::sort(m_index.begin(), m_index.end(), [](const auto& a, const auto& b)
		{
			const int first = stricmp(a.first.c_str(), b.first.c_str());
			if (first != 0)
			{
				return first < 0;
			}

			return stricmp(a.second.path.c_str(), b.second.path.c_str()) < 0;
		});
	});
}

std::pair<CDependencyTracker::IndexIterator, CDependencyTracker::IndexIterator> CDependencyTracker::GetRange(const char* szAssetPath) const
{
	using namespace Private_DependencyTracker;

	if (m_future.valid())
	{
		m_future.wait();
	}

	const string key = PathUtil::ToUnixPath(szAssetPath);
	const auto lowerBound = std::lower_bound(m_index.cbegin(), m_index.cend(), key, SLessKey());
	const auto upperBound = std::upper_bound(lowerBound, m_index.cend(), key, SLessKey());
	return { lowerBound, upperBound };
}
