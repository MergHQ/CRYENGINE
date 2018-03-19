// Copyright 2001-2016 Crytek GmbH. All rights reserved.

#pragma once

#include "AssetManagerHelpers.h"
#include <CrySystem/File/IFileChangeMonitor.h>

class CProgressNotification;

namespace AssetManagerHelpers
{

// ! Creates and updates cryassets for asset data files.
class CAssetGenerator : public IFileChangeListener
{
public:
	static void RegisterFileListener();
	virtual void OnFileChange(const char* szFilename, EChangeType changeType) override;

	//! Generates/repair *.cryasset files for the current project.
	static bool GenerateCryassets();

private:
	CAssetGenerator();
	void GenerateCryasset(const string& filePath);

private:
	CProcessingQueue m_fileQueue;
	string m_rcSettings;
	std::unique_ptr<CProgressNotification> m_pProgress;
};

};
