// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CObjectLayer;
class CUsedResources;

/*! Implements exporting of all loaded resources to specified directory.
 *
 */
class CGameResourcesExporter
{
public:
	void        ChooseDirectoryAndSave();
	void        ChooseDirectory();
	void        Save(const string& outputDirectory);

	void        GatherAllLoadedResources();
	void        SetUsedResources(CUsedResources& resources);

	void        SaveLayerResources(const string& subDirectory, CObjectLayer* pLayer, bool bAllChilds);

	static void ExportSelectedLayerResources();
	static void ExportPerLayerResourceList();

private:
	static void GatherLayerResourceList_r(CObjectLayer* pLayer, CUsedResources& resources);

private:
	typedef std::vector<string> Files;
	static Files m_files;

	string       m_path;

	//////////////////////////////////////////////////////////////////////////
	// Functions that gather files from editor subsystems.
	//////////////////////////////////////////////////////////////////////////
	void GetFilesFromObjects();
	void GetFilesFromMaterials();
	void GetFilesFromParticles();
};
