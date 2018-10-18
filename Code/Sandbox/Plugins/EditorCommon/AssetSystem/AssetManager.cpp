// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetManager.h"

#include "AssetImporter.h"
#include "PakImporter.h"
#include "AssetGenerator.h"
#include "AssetFileMonitor.h"
#include "AssetManagerHelpers.h"
#include "EditableAsset.h"
#include "DependencyTracker.h"
#include "FilePathUtil.h"
#include "ThreadingUtils.h"

#include "Browser/AssetModel.h"
#include "Browser/AssetFoldersModel.h"
#include "Loader/AssetLoaderBackgroundTask.h"
#include "Loader/AssetLoaderHelpers.h"
#include "AssetResourceSelector.h"
#include "SourceFilesTracker.h"
#include "FileOperationsExecutor.h"
#include "IFilesGroupProvider.h"
#include "AssetFilesGroupProvider.h"

#include <CrySystem/IConsole.h>
#include <CryString/CryPath.h>

namespace Private_AssetManager
{
using FileToAssetMap = std::unordered_map<string, CAsset*, stl::hash_stricmp<string>, stl::hash_stricmp<string>>;

void DeleteAssetFilesFromMap(FileToAssetMap& fileToAssetMap, CAsset* pAsset, bool shouldDeleteMetadataFile = true)
{
	if (shouldDeleteMetadataFile)
	{
		fileToAssetMap.erase(pAsset->GetMetadataFile());
	}
	for (auto const& file : pAsset->GetFiles())
	{
		fileToAssetMap.erase(file);
	}
}

void AddAssetFilesToMap(FileToAssetMap& fileToAssetMap, CAsset* pAsset, bool shouldAddMetadataFile = true)
{
	if (shouldAddMetadataFile)
	{
		fileToAssetMap[pAsset->GetMetadataFile()] = pAsset;
	}
	for (auto const& file : pAsset->GetFiles())
	{
		fileToAssetMap[file] = pAsset;
	}
}

void InitializeFileToAssetMap(FileToAssetMap& fileToAssetMap, const std::vector<CAsset*>& assets)
{
	// going once through the list of all assets in the beginning is probably not that expensive
	// comparing to penalty that we can incur by rehashing the whole table if the proper number
	// of elements is not reserved upfront. Doing some king of heuristic would be imprecise here.
	int numFiles = 0;
	for (auto pAsset : assets)
	{
		numFiles += pAsset->GetFilesCount() + 1;
	}
	const auto allocNum = numFiles * 1.1;   // buffer of 10%
	fileToAssetMap.reserve(allocNum);
}

	std::future<void> g_asyncProcess;

	std::vector<std::unique_ptr<IFilesGroupProvider>> ComposeFileGroupsForDeletion(std::vector<CAsset*>& assets)
	{
		std::vector<std::unique_ptr<IFilesGroupProvider>> fileGroups;
		fileGroups.reserve(assets.size());

		std::sort(assets.begin(), assets.end(), [](CAsset* pAsset1, CAsset* pAsset2)
		{
			return pAsset1->GetSourceFile() == pAsset2->GetSourceFile();
		});

		auto rangeStart = assets.begin();
		auto rangeEnd = rangeStart + 1;
		auto it = rangeStart;
		string sourceFile = (*rangeStart)->GetSourceFile();
		while (true)
		{
			if (rangeEnd == assets.end() || (*rangeEnd)->GetSourceFile() != sourceFile)
			{
				bool shouldIncludeSource = !sourceFile.empty();
				if (shouldIncludeSource)
				{
					auto count = CSourceFilesTracker::GetInstance()->GetCount(sourceFile);
					auto dist = std::distance(rangeStart, rangeEnd);
					shouldIncludeSource = count == dist;
				}
				fileGroups.emplace_back(new CAssetFilesGroupProvider(*it, shouldIncludeSource));
				if (rangeEnd == assets.end())
				{
					break;
				}
				rangeStart = rangeEnd;
				sourceFile = (*rangeStart)->GetSourceFile();
			}
			else
			{
				fileGroups.emplace_back(new CAssetFilesGroupProvider(*it, false));
			}
			++rangeEnd;
			++it;
		}
		return fileGroups;
	}
}

const std::pair<const char*, const char*> CAssetManager::m_knownAliases[] =
{
	{ "%engine%", "Engine" } };

CAssetManager* CAssetManager::s_instance = nullptr;

CAssetManager::CAssetManager()
	: m_assetModel(nullptr)
	, m_assetFoldersModel(nullptr)
	, m_isScanning(false)
	, m_orderedByGUID(false)
{
	s_instance = this;
	m_assetModel = new CAssetModel();
}

CAssetManager::~CAssetManager()
{
	delete m_assetModel;
	delete m_assetFoldersModel;

	s_instance = nullptr;
}

void CAssetManager::Init()
{
	using namespace AssetManagerHelpers;

	LOADING_TIME_PROFILE_SECTION;

	UpdateAssetTypes();
	UpdateAssetImporters();
	UpdateAssetConverters();
	RegisterAssetResourceSelectors();

	m_assetModel->Init();
	m_assetFoldersModel = new CAssetFoldersModel();
	m_pDependencyTracker.reset(new CDependencyTracker());

	AsyncScanForAssets();

	CAssetFileMonitor::RegisterFileListener();
	CAssetGenerator::RegisterFileListener();
	CPakImporter::RegisterFileListener();

	REGISTER_INT("ed_enableAssetPickers", 1, 0,
		"Value of 1 enables asset pickers for recommended types."
		"Value of 2 enables asset pickers for all types."
		"Any other value disables asset pickers for all types.");

	GetIEditor()->GetGlobalBroadcastManager()->Connect(BroadcastEvent::AboutToQuit, [this](BroadcastEvent& event)
	{
		CRY_ASSERT(event.type() == BroadcastEvent::AboutToQuit);
		
		std::vector<string> changedFiles;
		for (CAsset* pAsset : m_assets)
		{
			if (pAsset->IsModified() && !pAsset->IsBeingEdited())
			{
				changedFiles.push_back(pAsset->GetName());
			}
		}
		if (!changedFiles.empty())
		{
			AboutToQuitEvent& aboutToQuitEvent = static_cast<AboutToQuitEvent&>(event);
			aboutToQuitEvent.AddChangeList("Asset browser", changedFiles);

			event.ignore();
		}
		
	}, (uintptr_t)this);
}

CAssetType* CAssetManager::FindAssetType(const char* name) const
{
	auto it = std::lower_bound(m_assetTypes.cbegin(), m_assetTypes.cend(), name, [](const auto a, const auto b)
	{
		return strcmp(a->GetTypeName(), b) < 0;
	});

	return ((it != m_assetTypes.cend()) && (strcmp(name, (*it)->GetTypeName()) == 0)) ? *it : nullptr;
}

const std::vector<CAssetType*>& CAssetManager::GetAssetTypes() const
{
	return m_assetTypes;
}

const std::vector<CAssetImporter*>& CAssetManager::GetAssetImporters() const
{
	return m_assetImporters;
}

CAssetImporter* CAssetManager::GetAssetImporter(const string& ext, const string& assetTypeName) const
{
	// Validation in UpdateAssetImporters makes sure there is a single importer for this combination of ext,assetTypeName.
	for (CAssetImporter* pAssetImporter : m_assetImporters)
	{
		const std::vector<string>& exts = pAssetImporter->GetFileExtensions();
		const std::vector<string>& types = pAssetImporter->GetAssetTypes();
		if (std::find(exts.cbegin(), exts.cend(), ext) != exts.cend() && std::find(types.cbegin(), types.cend(), assetTypeName) != types.cend())
		{
			return pAssetImporter;
		}
	}
	return nullptr;
}

CAssetConverter* CAssetManager::GetAssetConverter(const QMimeData& data) const
{
	for (CAssetConverter* converter : m_assetConverters)
	{
		if (converter->CanConvert(data))
		{
			return converter;
		}
	}

	return nullptr;
}

bool CAssetManager::HasAssetConverter(const QMimeData& data) const
{
	return GetAssetConverter(data) != nullptr;
}

CAsset* CAssetManager::FindAssetForMetadata(const char* szFilePath) const
{
	MAKE_SURE(szFilePath, return nullptr);

	auto it = m_fileToAssetMap.find(szFilePath);
	if (it != m_fileToAssetMap.end())
	{
		return it->second;
	}

	return nullptr;
}

CAsset* CAssetManager::FindAssetForFile(const char* szFilePath) const
{
	MAKE_SURE(szFilePath, return nullptr);

	auto it = m_fileToAssetMap.find(szFilePath);
	if (it != m_fileToAssetMap.end())
	{
		return it->second;
	}

	// try to find in aliases.
	if (szFilePath[0] != '%')
	{
		for (const auto& alias : m_knownAliases)
		{
			auto it = m_fileToAssetMap.find(PathUtil::Make(alias.first, szFilePath));
			if (it != m_fileToAssetMap.end())
			{
				return it->second;
			}
		}
	}

	return nullptr;
}

CAsset* CAssetManager::FindAssetById(const CryGUID& guid)
{
	if (!m_orderedByGUID)
	{
		signalBeforeAssetsUpdated();
		std::sort(m_assets.begin(), m_assets.end(), [](const CAsset* a, const CAsset* b)
		{
			return a->GetGUID() < b->GetGUID();
		});
		m_orderedByGUID = true;
		signalAfterAssetsUpdated();
	}

	auto it = std::lower_bound(m_assets.cbegin(), m_assets.cend(), guid, [](const CAsset* pAsset, const CryGUID& guid)
	{
		return pAsset->GetGUID() < guid;
	});

	return it != m_assets.cend() && (*it)->GetGUID() == guid ? *it : nullptr;
}

void CAssetManager::AsyncScanForAssets()
{
	m_isScanning = true;

	std::vector<string> paths { PathUtil::GetGameFolder() };
	std::transform(std::cbegin(m_knownAliases), std::cend(m_knownAliases), std::back_inserter(paths), [](const auto& x)
	{
		return x.first;
	});
	AssetLoaderBackgroundTask::Run(paths, [this](const std::vector<CAsset*>& assets)
	{
		using namespace Private_AssetManager;
		m_isScanning = false;
		InitializeFileToAssetMap(m_fileToAssetMap, assets);
		InsertAssets(assets);
		MergeAssets(m_deferredInsertBatch);
		m_deferredInsertBatch.clear();
		signalScanningCompleted();
	});
}

void CAssetManager::InsertAssets(const std::vector<CAsset*>& assets)
{
	using namespace Private_AssetManager;
	if (assets.empty())
	{
		return;
	}

	if (m_isScanning)
	{
		m_deferredInsertBatch.insert(m_deferredInsertBatch.end(), assets.begin(), assets.end());
		return;
	}

	signalBeforeAssetsInserted(assets);

	m_assets.reserve(m_assets.size() + assets.size());

	for (CAsset* pAsset : assets)
	{
		m_assets.push_back(pAsset);
		m_orderedByGUID = false;
		AddAssetFilesToMap(m_fileToAssetMap, pAsset);
	}

	signalAfterAssetsInserted(assets);
}

void CAssetManager::MergeAssets(std::vector<CAsset*> assets)
{
	using namespace Private_AssetManager;
	if (m_isScanning)
	{
		m_deferredInsertBatch.insert(m_deferredInsertBatch.end(), assets.begin(), assets.end());
		return;
	}

	ICryPak* const pPak = GetISystem()->GetIPak();

	size_t count = assets.size();
	for (size_t i = 0; i < count;)
	{
		CAsset* const pOther = FindAssetForMetadata(assets[i]->GetMetadataFile());
		if (pOther)
		{
			if (pOther != assets[i])
			{
				// TODO: Better error handling.
				if (pOther->GetGUID() == assets[i]->GetGUID() && pOther->GetType() == assets[i]->GetType())
				{
					// TODO: Support of this is error prone. Probably we need something like an assignment operator.
					CEditableAsset editable(*pOther);
					editable.SetSourceFile(assets[i]->GetSourceFile());
					DeleteAssetFilesFromMap(m_fileToAssetMap, pOther, false);
					editable.SetFiles(assets[i]->GetFiles());
					AddAssetFilesToMap(m_fileToAssetMap, pOther, false);
					editable.SetLastModifiedTime(assets[i]->GetLastModifiedTime());
					editable.SetDependencies(assets[i]->GetDependencies());
					editable.SetDetails(assets[i]->GetDetails());

					// Force to update the thumbnail.
					if (!pOther->IsImmutable())
					{
						pPak->RemoveFile(pOther->GetThumbnailPath());
					}
					editable.InvalidateThumbnail();

					delete assets[i];
					assets[i] = nullptr;
				}
			}

			std::swap(assets[i], assets[--count]);

			// TODO: detect if list of files is changed, propagate the corresponding event eAssetChangeFlags_Files
			pOther->NotifyChanged(eAssetChangeFlags_All);

			// Do not increment index i.
			continue;
		}
		++i;
	}

	if (count)
	{
		assets.resize(count);
		InsertAssets(assets);
	}
}

bool CAssetManager::DeleteAssetsWithFiles(std::vector<CAsset*> assets)
{
	using namespace Private_AssetManager;
	MAKE_SURE(!m_assets.empty(), return false);
	MAKE_SURE(!assets.empty(), return false);

	auto fileGroups = ComposeFileGroupsForDeletion(assets);

	signalBeforeAssetsRemoved(assets);

	m_orderedByGUID = false;

	for (auto pAsset : assets)
	{
		pAsset->GetType()->PreDeleteAssetFiles(*pAsset);
	}

	g_asyncProcess = ThreadingUtils::AsyncQueue([assets = std::move(assets), fileGroups = std::move(fileGroups), this]() mutable
	{
		CFileOperationExecutor::GetExecutor()->Delete(std::move(fileGroups));

		ThreadingUtils::PostOnMainThread([this, assets = std::move(assets)]()
		{
			signalAfterAssetsRemoved();
		});
	});

	return true;
}

void CAssetManager::DeleteAssetsOnlyFromData(const std::vector<CAsset*>& assets)
{
	MAKE_SURE(!m_assets.empty(), return );
	MAKE_SURE(!assets.empty(), return );

	using namespace Private_AssetManager;
	signalBeforeAssetsRemoved(assets);
	int newSize = m_assets.size();
	for (auto pAsset : assets)
	{
		auto it = std::find(m_assets.cbegin(), m_assets.cend(), pAsset);
		if (it == m_assets.cend())
		{
			continue;
		}
		
		m_orderedByGUID = false;

		int index = std::distance(m_assets.cbegin(), it);
		const CAssetPtr pAssetToDelete = std::move(m_assets[index]);
		m_assets[index] = std::move(m_assets[--newSize]);
		m_assets.resize(newSize);
		DeleteAssetFilesFromMap(m_fileToAssetMap, pAssetToDelete);
	}
	signalAfterAssetsRemoved();
}

void CAssetManager::MoveAssets(const std::vector<CAsset*>& assets, const char* szDestinationFolder) const
{
	for (CAsset* pAsset : assets)
	{
		if (!pAsset)
		{
			continue;
		}

		const string newMetadataFile = PathUtil::Make(szDestinationFolder, PathUtil::GetFile(pAsset->GetMetadataFile()));
		if (CAssetManager::GetInstance()->FindAssetForMetadata(newMetadataFile))
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Asset already exists: %s", newMetadataFile.c_str());
			continue;
		}

		// Make a new copy of the source file if it is shared between several assets, move it to the new location otherwise.
		const bool bMoveSourceFile = !HasSharedSourceFile(*pAsset);
		pAsset->GetType()->MoveAsset(pAsset, szDestinationFolder, bMoveSourceFile);
	}
}

bool CAssetManager::RenameAsset(CAsset* pAsset, const char* szNewName)
{
	using namespace Private_AssetManager;

	DeleteAssetFilesFromMap(m_fileToAssetMap, pAsset);
	const bool result = pAsset->GetType()->RenameAsset(pAsset, szNewName);
	AddAssetFilesToMap(m_fileToAssetMap, pAsset);

	return result;
}

bool CAssetManager::HasSharedSourceFile(const CAsset& asset) const
{
	if (!asset.GetType()->IsImported())
	{
		return false;
	}

	return CSourceFilesTracker::GetInstance()->GetCount(asset.GetSourceFile()) > 1;
}

//Reimport is implemented in asset manager to avoid adding dependency from CAsset to CAssetImporter
void CAssetManager::Reimport(CAsset* pAsset)
{
	CRY_ASSERT(pAsset && pAsset->GetType() && !pAsset->IsImmutable());

	const char* szSourceFile = pAsset->GetSourceFile();
	if (!(szSourceFile && *szSourceFile))
	{
		return; // Empty source file, nothing to do.
	}

	string ext = PathUtil::GetExt(szSourceFile);
	ext.MakeLower();
	const char* const szAssetType = pAsset->GetType()->GetTypeName();
	CAssetImporter* const pAssetImporter = CAssetManager::GetInstance()->GetAssetImporter(ext, { szAssetType });
	if (!pAssetImporter)
	{
		return;
	}

	pAssetImporter->Reimport(pAsset);
}

std::vector<CAssetPtr> CAssetManager::GetAssetsFromDirectory(const string& directory, std::function<bool(CAsset*)> predicate /*={}*/) const
{
	std::vector<CAssetPtr> assets;
	assets.reserve(GetAssetsCount() / 8); // just a non zero assumption

	for (const CAssetPtr& pAsset : m_assets)
	{
		if (predicate && !predicate(pAsset))
		{
			continue;
		}

		if (directory.empty() || strncmp(directory.c_str(), pAsset->GetMetadataFile(), directory.size()) == 0)
		{
			assets.emplace_back(pAsset);
		}
	}
	return assets;
}

void CAssetManager::WaitAsyncProcess() const
{
	using namespace Private_AssetManager;
	if (g_asyncProcess.valid())
	{
		g_asyncProcess.wait();
	}
}

void CAssetManager::UpdateAssetTypes()
{
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_ASSET_TYPE, classes);

	m_assetTypes.clear();
	std::transform(classes.begin(), classes.end(), std::back_inserter(m_assetTypes), [](const IClassDesc* x) { return (CAssetType*)x; });

	// Validate asset type file extensions.
	// File extensions must...
	// ... be unique among asset types.
	// ... all lower-case.
	// ... not have a leading dot, nor a trailing "cryasset".
	{
		auto isLowerCase = [](const string& s)
		{
		 string lower = s;
		 lower.MakeLower();
		 return s == lower;
		};

		auto endsWithCryasset = [](const string& s)
		{
			size_t n = strlen("cryasset");
			return s.size() >= n && s.substr(s.size() - n) == "cryasset";
		};

		for (CAssetType* pAssetType : m_assetTypes)
		{
			if (pAssetType->GetTypeName() == "cryasset")
			{
				// File extension of fall-back asset type can be arbitrary.
				continue;
			}

			const char* szExt = pAssetType->GetFileExtension();
			CRY_ASSERT_MESSAGE(szExt && *szExt, "File extension of type %s must not be empty", pAssetType->GetTypeName());
			CRY_ASSERT_MESSAGE(szExt[0] != '.', "File extension of type %s must not have leading dot", pAssetType->GetTypeName());
			CRY_ASSERT_MESSAGE(isLowerCase(szExt), "File extension of type %s must be all-lower case", pAssetType->GetTypeName());
			CRY_ASSERT_MESSAGE(!endsWithCryasset(szExt), "File extension of type %s must be all-lower case", pAssetType->GetTypeName());
		}

		std::sort(m_assetTypes.begin(), m_assetTypes.end(), [](const auto a, const auto b)
		{
			return strcmp(a->GetFileExtension(), b->GetFileExtension()) < 0;
		});

		auto it = std::adjacent_find(m_assetTypes.begin(), m_assetTypes.end(), [](const auto a, const auto b)
		{
			return strcmp(a->GetFileExtension(), b->GetFileExtension()) == 0;
		});

		if (it != m_assetTypes.end())
		{
			CRY_ASSERT_MESSAGE(false, "Two asset types cannot be registered with the same file extension : %s", (*it)->GetFileExtension());
		}
	}

	// Validate asset type names.
	// Type names must be unique among asset types.
	{
		std::sort(m_assetTypes.begin(), m_assetTypes.end(), [](const auto a, const auto b)
		{
			return strcmp(a->GetTypeName(), b->GetTypeName()) < 0;
		});

		auto it = std::adjacent_find(m_assetTypes.begin(), m_assetTypes.end(), [](const auto a, const auto b)
		{
			return strcmp(a->GetTypeName(), b->GetTypeName()) == 0;
		});

		if (it != m_assetTypes.end())
		{
			CRY_ASSERT_MESSAGE(false, "Two asset types cannot be registered with the same type name : %s", (*it)->GetTypeName());
		}
	}

	// CAssetManager::FindAssetType() assumes types to be sorted by name.
	CRY_ASSERT(std::is_sorted(m_assetTypes.begin(), m_assetTypes.end(), [](const auto a, const auto b)
	{
		return strcmp(a->GetTypeName(), b->GetTypeName()) < 0;
	}));

	for (CAssetType* pAssetType : m_assetTypes)
	{
		pAssetType->Init();
	}
}

void CAssetManager::UpdateAssetImporters()
{
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_ASSET_IMPORTER, classes);

	m_assetImporters.clear();
	m_assetImporters.reserve(classes.size());
	std::transform(classes.begin(), classes.end(), std::back_inserter(m_assetImporters), [](const IClassDesc* x) { return (CAssetImporter*)x; });

	//! TODO: Validate asset importers.
}

void CAssetManager::UpdateAssetConverters()
{
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_ASSET_COVERTER, classes);

	m_assetConverters.clear();
	m_assetConverters.reserve(classes.size());
	std::transform(classes.begin(), classes.end(), std::back_inserter(m_assetConverters), [](const IClassDesc* x) { return (CAssetConverter*)x; });
}

void CAssetManager::RegisterAssetResourceSelectors()
{
	m_resourceSelectors.clear();
	m_resourceSelectors.reserve(m_assetTypes.size());
	for (CAssetType* assetType : m_assetTypes)
	{
		if (assetType->IsUsingGenericPropertyTreePicker())
			m_resourceSelectors.emplace_back(assetType->GetTypeName());
	}

	for (const SStaticAssetSelectorEntry& entry : m_resourceSelectors)
	{
		GetIEditor()->GetResourceSelectorHost()->RegisterResourceSelector(&entry);
	}
}

std::vector<std::pair<CAsset*, int32>> CAssetManager::GetReverseDependencies(const CAsset& asset) const
{
	MAKE_SURE(asset.GetFile(0), return {});

	std::vector<SAssetDependencyInfo> dependencyInfos = m_pDependencyTracker.get()->GetReverseDependencies(asset.GetFile(0));
	std::vector<std::pair<CAsset*, int32>> dependencies;
	dependencies.reserve(dependencyInfos.size());

	for (const auto& item : dependencyInfos)
	{
		CAsset* pAsset = CAssetManager::GetInstance()->FindAssetForFile(item.path);
		if (!pAsset)
		{
			continue;
		}
		dependencies.emplace_back(pAsset, item.usageCount);
	}
	return dependencies;
}

std::vector<CAsset*> CAssetManager::GetReverseDependencies(const std::vector<CAsset*>& assets) const
{
	std::vector<CAsset*> ordered(assets);
	std::sort(ordered.begin(), ordered.end());
	std::unordered_set<CAsset*> set;
	for (CAsset* pAsset : ordered)
	{
		if (!pAsset)
		{
			continue;
		}
		std::vector<std::pair<CAsset*, int32>> tmp = GetReverseDependencies(*pAsset);
		std::transform(tmp.begin(), tmp.end(), std::inserter(set, set.begin()), [](auto& x)
		{
			return x.first;
		});
	}
	std::vector<CAsset*> dependencies;
	dependencies.reserve(set.size());

	// The given assets are not considered as dependent assets.
	std::copy_if(set.begin(), set.end(), std::back_inserter(dependencies), [&ordered](const CAsset* x)
	{
		return !std::binary_search(ordered.begin(), ordered.end(), x);
	});
	return dependencies;
}

bool CAssetManager::HasAnyReverseDependencies(const CAsset& asset) const
{
	MAKE_SURE(asset.GetFile(0), return false);

	std::vector<SAssetDependencyInfo> dependencyInfos = m_pDependencyTracker.get()->GetReverseDependencies(asset.GetFile(0));

	for (const auto& item : dependencyInfos)
	{
		CAsset* pAsset = CAssetManager::GetInstance()->FindAssetForFile(item.path);
		if (pAsset)
		{
			return true;
		}
	}
	return false;
}

bool CAssetManager::HasAnyReverseDependencies(const std::vector<CAsset*>& assets) const
{
	return std::any_of(assets.cbegin(), assets.cend(), [&assets, this](CAsset* pAsset)
	{
		if (!pAsset)
		{
		  return false;
		}

		std::vector<std::pair<CAsset*, int32>> dependencyInfos(GetReverseDependencies(*pAsset));
		if (dependencyInfos.empty())
		{
		  return false;
		}

		// The given assets are not considered as dependent assets.
		return std::any_of(dependencyInfos.cbegin(), dependencyInfos.cend(), [&assets](const auto& item)
		{
			return std::find(assets.cbegin(), assets.cend(), item.first) == assets.end();
		});
	});
}

const char* CAssetManager::GetAliasName(const char* alias) const
{
	const auto it = std::find_if(std::cbegin(m_knownAliases), std::cend(m_knownAliases), [alias](const auto& x)
	{
		return stricmp(x.first, alias) == 0;
	});

	return (it != std::end(m_knownAliases)) ? it->second : alias;
}

std::vector<const char*> CAssetManager::GetAliases() const
{
	std::vector<const char*> aliases;
	std::transform(std::cbegin(m_knownAliases), std::cend(m_knownAliases), std::back_inserter(aliases), [](const auto& x)
	{
		return x.first;
	});
	return aliases;
}

void CAssetManager::GenerateCryassetsAsync(const std::function<void()>& finalize)
{
	using namespace AssetManagerHelpers;

	m_isScanning = true;

	ThreadingUtils::Async([this, finalize]()
	{
		using namespace AssetManagerHelpers;

		CAssetGenerator::GenerateCryassets();

		if (GetIEditor()->IsMainFrameClosing())
		{
		  return;
		}

		ThreadingUtils::PostOnMainThread([this, finalize]()
		{
			if (GetIEditor()->IsMainFrameClosing())
			{
			  return;
			}

			m_isScanning = false;
			MergeAssets(m_deferredInsertBatch);
			m_deferredInsertBatch.clear();

			if (finalize)
			{
			  finalize();
			}
		});
	});
}

void CAssetManager::SaveAll(std::function<void(float)> progress)
{
	if (!progress)
	{
		progress = [](float) {};
	}

	for (size_t i = 0, n = m_assets.size(); i < m_assets.size(); ++i)
	{
		CAssetPtr& pAsset = m_assets[i];

		// TODO: not all asset editors maintain the IsModified() state properly.
		if (pAsset->IsModified())
		{
			progress(float(i) / n);
		}
		pAsset->Save();
	}
	progress(1.0f);
}

void CAssetManager::SaveBackup(const string& backupFolder)
{
	for (CAssetPtr& pAsset : m_assets)
	{
		// TODO: not all asset editors maintain the IsModified() state properly.
		if (/* pAsset->IsModified() && */ pAsset->GetCurrentEditor())
		{
			pAsset->GetCurrentEditor()->SaveBackup(backupFolder);
		}
	}
}
