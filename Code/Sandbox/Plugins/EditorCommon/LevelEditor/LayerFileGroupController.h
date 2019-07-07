// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AssetSystem/IFilesGroupController.h"

struct IObjectLayer;

//! This class is responsible for providing files that comprise a particular layer.
class CLayerFileGroupController : public IFilesGroupController
{
public:
	explicit CLayerFileGroupController(IObjectLayer& layer);

	virtual std::vector<string> GetFiles(bool includeGeneratedFile /*= true*/) const override final;

	virtual const string& GetMainFile() const override final { return m_mainFile; }

	virtual const string& GetName() const override final { return m_name; }

	virtual void Update() override final;

private:
	IObjectLayer* m_pLayer;
	string        m_mainFile;
	string        m_name;
};
