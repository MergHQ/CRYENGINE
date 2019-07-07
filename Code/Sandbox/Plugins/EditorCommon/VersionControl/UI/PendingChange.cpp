// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PendingChange.h"

#include "VersionControl/AssetFilesProvider.h"
#include "VersionControl/AssetsVCSStatusProvider.h"
#include "VersionControl/VersionControl.h"
#include "AssetSystem/Loader/Metadata.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "FileUtils.h"
#include "PathUtils.h"
#include "AssetSystem/AssetManager.h"
#include "Util/EditorUtils.h"
#include <CrySystem/XML/IXml.h>

namespace Private_PendingChange
{

static std::vector<std::unique_ptr<CPendingChange>> g_pendingChanges;

//! Looks for a pending change that corresponds to the given file handle.
std::vector<std::unique_ptr<CPendingChange>>::const_iterator FindInPendingChanges(const string& mainFile)
{
	return std::find_if(g_pendingChanges.cbegin(), g_pendingChanges.cend(), [&mainFile](const auto& pPendingChange)
	{
		return stricmp(pPendingChange->GetMainFile().c_str(), mainFile.c_str()) == 0;
	});
}

bool DoesFileExist(const string& file)
{
	return FileUtils::FileExists(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), file));
}

bool IsLayerFile(const char* fileName)
{
	const char* const szExt = PathUtil::GetExt(fileName);
	return strcmp(szExt, "lyr") == 0;
}

//! This class is responsible for retrieving a file's content from VCS repository.
//! It makes sure that if the object is destroyed the retrieving (async) task is canceled.
class CFilesContentRetriever
{
public:
	CFilesContentRetriever(const string& file)
		: m_file(file)
	{}

	~CFilesContentRetriever()
	{
		// if the pending change is deleted but retrieve task is still running cancel it.
		if (auto pTask = m_retrieveContentTask.lock())
		{
			pTask->Cancel();
		}
	}

	//! Retrieves the file's content from VCS repository if missing in cache and calls a callback upon complete.
	void Retrieve(std::function<void(const string&)> callback)
	{
		const string& filesContent = CVersionControl::GetInstance().GetFilesContent(m_file);
		if (!filesContent.empty())
		{
			callback(filesContent);
			return;
		}

		m_retrieveContentTask = CVersionControl::GetInstance().RetrieveFilesContent(m_file, false
			, [this, callback = std::move(callback)](const auto& result)
		{
			callback(CVersionControl::GetInstance().GetFilesContent(m_file));
		});
	}

private:
	string m_file;
	std::weak_ptr<CVersionControlTask> m_retrieveContentTask;
};

//! Specific implementation of pending change for non-level assets.
class CPendingAssetChange : public CPendingChange
{
public:
	explicit CPendingAssetChange(CAsset& asset)
	{
		m_name = QtUtil::ToQString(asset.GetName()); 
		m_typeName = asset.GetType()->GetUiTypeName(); 
		m_location = QtUtil::ToQString(asset.GetFolder()); 
		m_mainFile = asset.GetMetadataFile();
		m_files = CAssetFilesProvider::GetForAssets({ &asset });
	}

	virtual bool IsValid() const { return DoesFileExist(m_mainFile); }
};

//! Specific implementation of pending deletion for non-level assets.
class CPendingDeletionAssetChange : public CPendingChange
{
public:
	explicit CPendingDeletionAssetChange(string metadataFile)
		: m_filesContentRetriever(metadataFile)
	{
		m_location = QtUtil::ToQString(PathUtil::MakeGamePath(PathUtil::GetParentDirectory(metadataFile)));
		m_name = QtUtil::ToQString(PathUtil::GetFileName(metadataFile));
		m_typeName = "...";
		m_mainFile = metadataFile;

		m_filesContentRetriever.Retrieve([this](const string& filesContent)
		{
			ReadAndApplyAssetsFileContent(filesContent);
		});
	}

private:
	void ReadAndApplyAssetsFileContent(const string& filesContent)
	{
		AssetLoader::SAssetMetadata metadata;

		XmlNodeRef container = GetISystem()->LoadXmlFromBuffer(filesContent, filesContent.size());
		if (container && AssetLoader::ReadMetadata(container, metadata))
		{
			std::vector<string> files;
			files.reserve(metadata.files.size() + 2);
			files.push_back(GetMainFile());
			std::transform(metadata.files.cbegin(), metadata.files.cend(), std::back_inserter(files),
				[this](const string& file)
			{
				return PathUtil::Make(QtUtil::ToString(m_location), file);
			});
			m_typeName = QtUtil::ToQString(metadata.type);
			CAssetType* pType = GetIEditor()->GetAssetManager()->FindAssetType(metadata.type);
			if (pType && pType->HasThumbnail())
			{
				files.push_back(PathUtil::ReplaceExtension(GetMainFile(), "thmb.png"));
			}
			m_files = std::move(files);
		}
	}

	CFilesContentRetriever m_filesContentRetriever;
};

//! Specific implementation of pending change for levels.
class CPendingLevelChange : public CPendingAssetChange
{
public:
	explicit CPendingLevelChange(CAsset& asset)
		: CPendingAssetChange(asset)
	{
		auto extPos = GetMainFile().rfind(".level.cryasset");
		m_levelFolder = GetMainFile().substr(0, extPos);
		m_files = CAssetFilesProvider::GetForAssets({ &asset }, CAssetFilesProvider::EInclude::AllFiles, true);
	}

	virtual void Check(bool shouldCheck) override
	{
		using namespace Private_PendingChange;
		CPendingAssetChange::Check(shouldCheck);

		if (!CAssetsVCSStatusProvider::HasStatus(GetMainFile(), CVersionControlFileStatus::eState_AddedLocally))
		{
			return;
		}

		std::vector<CPendingChange*> updatedPendingChanges;

		for (const auto& pPendingChange : g_pendingChanges)
		{
			if (pPendingChange->IsChecked() != shouldCheck && IsLevelsLayer(pPendingChange.get()))
			{
				pPendingChange->SetChecked(shouldCheck);
				updatedPendingChanges.push_back(pPendingChange.get());
			}
		}
		if (!updatedPendingChanges.empty())
		{
			s_signalDataUpdatedIndirectly(updatedPendingChanges);
		}
	}

	virtual void SetChecked(bool value)
	{
		CPendingAssetChange::SetChecked(value);
		RemoveLayersFiles();
	}

	virtual bool IsValid() const { return DoesFileExist(m_mainFile); }

private:
	void RemoveLayersFiles()
	{
		if (m_areLayerFilesRemoved)
		{
			return;
		}
		m_areLayerFilesRemoved = true;
		auto layersStartIt = std::partition(m_files.begin(), m_files.end(), [](const string& file)
		{
			return strcmp(PathUtil::GetExt(file), "lyr") != 0;
		});
		std::vector<string> layersFiles;
		for (auto it = layersStartIt; it != m_files.end(); ++it)
		{
			auto layerIt = FindInPendingChanges(*it);
			if (layerIt != g_pendingChanges.end())
			{
				std::vector<string> layerFiles = (*layerIt)->GetFiles();
				std::move(layerFiles.begin(), layerFiles.end(), std::back_inserter(layersFiles));
			}
		}
		if (!layersFiles.empty())
		{
			m_files.erase(layersStartIt, m_files.end());
			std::sort(m_files.begin(), m_files.end());
			std::sort(layersFiles.begin(), layersFiles.end());
			std::vector<string> tmpFiles = std::move(m_files);
			m_files.clear();
			std::set_difference(tmpFiles.cbegin(), tmpFiles.cend(), layersFiles.cbegin(), layersFiles.cend(), std::back_inserter(m_files));
		}
	}

	bool IsLevelsLayer(const CPendingChange* pPendingChange)
	{
		return pPendingChange->GetMainFile().compareNoCase(0, m_levelFolder.size(), m_levelFolder) == 0;
	}

	string m_levelFolder;
	bool m_areLayerFilesRemoved{ false };
};

//! Specific implementation of pending change for layers.
class CPendingLayerChange : public CPendingChange
{
public:
	explicit CPendingLayerChange(const string& layerFile)
	{
		m_typeName = "Layer";
		m_location = PathUtil::GetPathWithoutFilename(layerFile);
		m_name = PathUtil::GetFileName(layerFile);
		string lowerPath = layerFile;
		lowerPath.MakeLower();
		int layersPos = lowerPath.find("/layers/");
		int levelPos = layerFile.rfind('/', layersPos - 1);
		levelPos = levelPos == std::string::npos ? 0 : levelPos + 1;
		m_name = QString("%1 (%2)").arg(m_name, layerFile.substr(levelPos, layersPos - levelPos).c_str());
		m_levelPath = layerFile.substr(0, layersPos) + ".level.cryasset";
		m_mainFile = layerFile;
		m_files = { layerFile };
	}

	virtual void SetChecked(bool value) override
	{
		CPendingChange::SetChecked(value);
		if (IsChecked() && m_shouldReadXmlFile)
		{
			m_shouldReadXmlFile = false;
			ReadFilesFromXmlAndAdd();
		}
	}

	virtual void Check(bool shouldCheck) override
	{
		using namespace Private_PendingChange;
		CPendingChange::Check(shouldCheck);
		if (!shouldCheck)
		{
			return;
		}
		auto it = FindInPendingChanges(m_levelPath);
		if (it != g_pendingChanges.end() && !(*it)->IsChecked()
			&& CVersionControl::GetInstance().GetFileStatus(m_levelPath)->HasState(CVersionControlFileStatus::eState_AddedLocally))
		{
			(*it)->SetChecked(true);
			s_signalDataUpdatedIndirectly({ (*it).get() });
		}
	}

	virtual bool IsValid() const { return DoesFileExist(m_mainFile); }

protected:
	void ReadFilesFromXmlAndAdd()
	{
		XmlNodeRef pRoot = XmlHelpers::LoadXmlFromFile(m_mainFile);
		if (pRoot)
		{
			ReadFilesFromXmlAndAdd(pRoot);
		}
	}

	void ReadFilesFromXmlAndAdd(XmlNodeRef pRoot)
	{
		XmlNodeRef pLayerDesc = pRoot->findChild("Layer");
		XmlNodeRef pFiles = pLayerDesc->findChild("Files");
		if (pFiles)
		{
			const string levelFolder = PathUtil::GetParentDirectory(PathUtil::GetDirectory(m_mainFile));
			for (int i = 0, n = pFiles->getChildCount(); i < n; ++i)
			{
				XmlNodeRef pFile = pFiles->getChild(i);
				m_files.emplace_back(PathUtil::MakeGamePath(PathUtil::Make(levelFolder, pFile->getAttr("path"))));
			}
		}
	}

	string m_levelPath;
	bool m_shouldReadXmlFile{ true };
};

//! Specific implementation of pending deletion for a layer.
class CPendingDeletionLayerChange : public CPendingLayerChange
{
public:
	explicit CPendingDeletionLayerChange(const string& layerFile)
		: CPendingLayerChange(layerFile)
		, m_filesContentRetriewer(layerFile)
	{
		m_shouldReadXmlFile = false;
		m_filesContentRetriewer.Retrieve([this](const string& filesContent)
		{
			ReadAndApplyLayersFileContent(filesContent);
		});
	}

private:
	void ReadAndApplyLayersFileContent(const string& filesContent)
	{
		XmlNodeRef pRoot = GetISystem()->LoadXmlFromBuffer(filesContent, filesContent.size());
		if (pRoot)
		{
			ReadFilesFromXmlAndAdd(pRoot);
		}
	}

	CFilesContentRetriever m_filesContentRetriewer;
};

//! Specific implementation of pending deletion for a level.
class CPendingDeletionLevelChange : public CPendingChange
{
public:
	explicit CPendingDeletionLevelChange(const string& levelFile)
	{
		m_typeName = "Level";
		m_location = QtUtil::ToQString(PathUtil::MakeGamePath(PathUtil::GetParentDirectory(levelFile)));
		m_name = QtUtil::ToQString(PathUtil::GetFileName(levelFile));
		m_mainFile = levelFile;
	}
};

//! Specific implementation of pending change for a work file.
class CPendingWorkFileChange : public CPendingChange
{
public:
	explicit CPendingWorkFileChange(const string& workFile)
	{
		m_typeName = "-";
		m_location = PathUtil::GetPathWithoutFilename(workFile);
		m_name = QtUtil::ToQString(PathUtil::GetFile(workFile));
		m_mainFile = workFile;
		m_files = { workFile };
	}
};

}

CCrySignal<void(const std::vector<CPendingChange*>&)> CPendingChange::s_signalDataUpdatedIndirectly;

CPendingChange* CPendingChangeList::GetPendingChange(int index)
{
	return Private_PendingChange::g_pendingChanges[index].get();
}

int CPendingChangeList::GetNumPendingChanges()
{
	return Private_PendingChange::g_pendingChanges.size();
}

CPendingChange* CPendingChangeList::FindPendingChangeFor(const string& file)
{
	using namespace Private_PendingChange;
	auto it = FindInPendingChanges(file);
	return it != g_pendingChanges.end() ? (*it).get() : nullptr;
}

CPendingChange* CPendingChangeList::CreatePendingChangeFor(const string& file)
{
	using namespace Private_PendingChange;
	if (AssetLoader::IsMetadataFile(file))
	{
		if (CAssetsVCSStatusProvider::HasStatus(file, CVersionControlFileStatus::eState_DeletedLocally))
		{
			if (strcmp(PathUtil::GetExt(PathUtil::GetFileName(file)), "level") == 0)
			{
				g_pendingChanges.push_back(std::make_unique<CPendingDeletionLevelChange>(file));
			}
			else
			{
				g_pendingChanges.push_back(std::make_unique<CPendingDeletionAssetChange>(file));
			}
		}
		else if (CAsset* pAsset = GetIEditor()->GetAssetManager()->FindAssetForMetadata(file))
		{
			if (strcmp(pAsset->GetType()->GetTypeName(), "Level") == 0)
			{
				g_pendingChanges.push_back(std::make_unique<CPendingLevelChange>(*pAsset));
			}
			else
			{
				g_pendingChanges.push_back(std::make_unique<CPendingAssetChange>(*pAsset));
			}
		}
		else 
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Version Control: Asset's file %s doesn't exist.", file.c_str());
			return nullptr;
		}
	}
	else if (IsLayerFile(file))
	{
		if (CAssetsVCSStatusProvider::HasStatus(file, CVersionControlFileStatus::eState_DeletedLocally))
		{
			g_pendingChanges.push_back(std::make_unique<CPendingDeletionLayerChange>(file));
		}
		else if (FileUtils::FileExists(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(),file)))
		{
			g_pendingChanges.push_back(std::make_unique<CPendingLayerChange>(file));
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Version Control: Layer's file %s doesn't exist.", file.c_str());
			return nullptr;
		}
	}
	else
	{
		if (CAssetsVCSStatusProvider::HasStatus(file, CVersionControlFileStatus::eState_DeletedLocally) ||
			FileUtils::FileExists(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), file)))
		{
			g_pendingChanges.push_back(std::make_unique<CPendingWorkFileChange>(file));
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Version Control: Work file %s doesn't exist.", file.c_str());
			return nullptr;
		}
	}
	return g_pendingChanges.back().get();
}

void CPendingChangeList::ForEachPendingChange(std::function<void(CPendingChange&)> f)
{
	using namespace Private_PendingChange;
	for (const auto& pPendingChange : g_pendingChanges)
	{
		f(*pPendingChange.get());
	}
}

void CPendingChangeList::Clear()
{
	Private_PendingChange::g_pendingChanges.clear();
}
