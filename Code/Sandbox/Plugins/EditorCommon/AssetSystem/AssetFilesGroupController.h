// Copyright 2001-2019 Crytek GmbH. All rights reserved.
#pragma once

#include "IFilesGroupController.h"
#include "CryString/CryString.h"
#include <vector>

class CAsset;

class CAssetFilesGroupController : public IFilesGroupController
{
public:
	CAssetFilesGroupController(CAsset* pAsset, bool shouldIncludeSourceFile);

	virtual std::vector<string> GetFiles(bool includeGeneratedFile = true) const override final;

	virtual const string& GetName() const override final { return m_name; }

	virtual const string& GetMainFile() const override final { return m_metadata; }

	virtual string GetGeneratedFile() const override final;

	virtual void Update() override final;

private:
	CAsset* m_pAsset;
	string  m_metadata;
	string  m_name;
	bool    m_shouldIncludeSourceFile;
};
