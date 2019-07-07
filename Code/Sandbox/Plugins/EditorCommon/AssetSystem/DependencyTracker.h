// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include <future> 

class CAsset;
struct SAssetDependencyInfo;

//! Dependency Tracker allows dependencies between assets to be tracked.
class EDITOR_COMMON_API CDependencyTracker
{
	friend class CAssetManager;
	CDependencyTracker();
public:
	virtual ~CDependencyTracker();

	//! Returns a list of assets that use the specified file.
	//! \param szAssetPath The asset to enumerate reverse dependencies.
	//! \return Returns a collection of SAssetDependencyInfo, where
	// the string part of each item is the path relative to the assets root directory, and 
	// the integer value contains the instance count for the dependency or 0 if such information is not available.
	std::vector<SAssetDependencyInfo> GetReverseDependencies(const char* szAssetPath) const;

	//! Tests if the specified asset is used by another asset.
	std::pair<bool,int> IsAssetUsedBy(const char* szAssetPath, const char* szAnotherAssetPath) const;

private:
	typedef std::vector<std::pair<string, SAssetDependencyInfo>>::const_iterator IndexIterator;

private:
	void CreateIndex();
	 std::pair<IndexIterator, IndexIterator> GetRange(const char* szAssetPath) const;

private:
	std::vector<std::pair<string, SAssetDependencyInfo>> m_index;
	std::future<void> m_future;
};
