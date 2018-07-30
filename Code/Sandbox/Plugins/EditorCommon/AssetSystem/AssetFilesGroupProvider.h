// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#pragma once

#include "IFilesGroupProvider.h"
#include "CryString/CryString.h"
#include <vector>

class CAsset;

class CAssetFilesGroupProvider : public IFilesGroupProvider
{
public:
	CAssetFilesGroupProvider(CAsset* pAsset, bool shouldIncludeSourceFile)
		: m_pAsset(pAsset)
		, m_metadata(pAsset->GetMetadataFile())
		, m_shouldIncludeSourceFile(shouldIncludeSourceFile)
	{}

	virtual std::vector<string> GetFiles() const override final;

	virtual const string& GetName() const override final;

	virtual const string& GetMainFile() const override final;

	virtual void Update() override final;

private:
	CAsset* m_pAsset;
	string  m_metadata;
	bool    m_shouldIncludeSourceFile;
};
