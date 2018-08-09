// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"

class CAsset;

class EDITOR_COMMON_API CSourceFilesTracker
{
public:
	static CSourceFilesTracker* GetInstance() 
	{
		static CSourceFilesTracker instance;
		return &instance;
	}

	void Add(const CAsset& asset);

	void Remove(const CAsset& asset);

	int GetCount(const string& sourceFile);

	std::vector<CAsset*> GetAssetForSourceFile(const string& sourceFile);

private:
	CSourceFilesTracker() {}
};
