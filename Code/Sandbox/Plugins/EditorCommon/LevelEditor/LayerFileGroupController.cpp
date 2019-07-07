// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "LayerFileGroupController.h"
#include "PathUtils.h"
#include "FileUtils.h"
#include "Objects/IObjectLayerManager.h"
#include "Objects/IObjectLayer.h"
#include "Objects/ObjectManager.h"
#include "IEditor.h"


namespace Private_LayerFileGroup
{

bool IsLayerOfOpenedLevel(const string& layerFile, string& outLevelPath)
{
	const string levelPath = GetIEditor()->GetLevelPath();
	if (levelPath.empty())
	{
		return false;
	}
	outLevelPath = PathUtil::MakeGamePath(levelPath);

	return layerFile.compareNoCase(0, outLevelPath.size(), outLevelPath) == 0;
}

}
CLayerFileGroupController::CLayerFileGroupController(IObjectLayer& layer)
	: m_pLayer(&layer)
{
	m_mainFile = PathUtil::ToGamePath(m_pLayer->GetLayerFilepath());
	m_name = m_pLayer->GetName();
}

std::vector<string> CLayerFileGroupController::GetFiles(bool includeGeneratedFile /*= true*/) const
{
	if (!m_pLayer)
	{
		return {};
	}

	std::vector<string> result;
	const auto& files = m_pLayer->GetFiles();
	result.reserve(files.size() + 1);
	result.push_back(GetMainFile());

	if (files.empty())
	{
		return result;
	}

	const string levelPath = PathUtil::ToGamePath(GetIEditor()->GetLevelPath());

	std::transform(files.cbegin(), files.cend(), std::back_inserter(result), [&levelPath](const string& file)
	{
		return PathUtil::Make(levelPath, file);
	});
	return result;
}

void CLayerFileGroupController::Update()
{
	using namespace Private_LayerFileGroup;
	IObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetIObjectLayerManager();
	string levelFolderPath;
	// first check if the level of this layer is still opened.
	if (!IsLayerOfOpenedLevel(m_mainFile, levelFolderPath))
	{
		CryLog("Layer file %s doesn't belong to current level in %s. So no layer reload is needed.", m_mainFile, levelFolderPath);
		return;
	}
	const string layersFolderPath = PathUtil::Make(levelFolderPath, "Layers");
	string fullName = m_mainFile.substr(layersFolderPath.size() + 1, m_mainFile.size() - layersFolderPath.size() - 5);
	m_pLayer = pLayerManager->FindLayerByFullName(fullName);
	CRY_ASSERT_MESSAGE(m_pLayer, "Layer %s could not be found in memory", m_mainFile);
	if (m_pLayer)
	{
		// if layer file exists we reload it
		if (FileUtils::FileExists(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), m_mainFile)))
		{
			CryLog("Executing reloading of layer file %s", m_mainFile);
			pLayerManager->ReloadLayer(m_pLayer);
			m_pLayer = pLayerManager->FindLayerByFullName(fullName);
			m_pLayer->SetModified(false);
		}
		// otherwise if the file is deleted we remove the layer from the level.
		else
		{
			CryLog("no reloading of layer file %s since it doesn't exist. So the layer is deleted from the current level.", m_mainFile);
			pLayerManager->DeleteLayer(m_pLayer, true, false);
			m_pLayer = nullptr;
		}
	}
}
