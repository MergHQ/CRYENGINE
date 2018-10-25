// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#include "StdAfx.h"
#include "SourceFilesTracker.h"
#include "Asset.h"
#include <CryCore/StlUtils.h>
#include <unordered_map>

namespace Private_SourceFilesTracker
{

std::unordered_multimap<string, const CAsset*, stl::hash_strcmp<string>, stl::hash_strcmp<string>> _sourceFiles;

auto Find(const CAsset& asset)
{
	auto range = _sourceFiles.equal_range(asset.GetSourceFile());
	auto it = std::find_if(range.first, range.second, [&asset](const auto& pair)
	{
		return pair.second == &asset;
	});
	return it != range.second ? it : _sourceFiles.end();
}

}

void CSourceFilesTracker::Add(const CAsset& asset)
{
	using namespace Private_SourceFilesTracker;
	auto it = Find(asset);
	CRY_ASSERT_MESSAGE(it == _sourceFiles.end(), "attempt to add already tracked source file %s", asset.GetSourceFile());
	if (it == _sourceFiles.end())
	{
		_sourceFiles.insert(std::make_pair(asset.GetSourceFile(), &asset));
	}
}

void CSourceFilesTracker::Remove(const CAsset& asset)
{
	using namespace Private_SourceFilesTracker;
	auto it = Find(asset);
	if (it != _sourceFiles.end())
	{
		_sourceFiles.erase(it);
		return;
	}
	CRY_ASSERT_MESSAGE(false, "attempt to remove untracked source file %s", asset.GetSourceFile());
}

int CSourceFilesTracker::GetCount(const string& sourceFile)
{
	using namespace Private_SourceFilesTracker;
	auto range = _sourceFiles.equal_range(sourceFile);
	return std::distance(range.first, range.second);
}

std::vector<CAsset*> CSourceFilesTracker::GetAssetForSourceFile(const string& sourceFile)
{
	using namespace Private_SourceFilesTracker;
	std::vector<CAsset*> assets;
	assets.reserve(GetCount(sourceFile));
	auto range = _sourceFiles.equal_range(sourceFile);
	for (auto it = range.first; it != range.second; ++it)
	{
		assets.push_back(const_cast<CAsset*>(it->second));
	}
	return assets;
}
