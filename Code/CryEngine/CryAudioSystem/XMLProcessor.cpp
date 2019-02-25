// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "XMLProcessor.h"
#include "Managers.h"
#include "FileCacheManager.h"
#include "LoseFocusTrigger.h"
#include "GetFocusTrigger.h"
#include "MuteAllTrigger.h"
#include "UnmuteAllTrigger.h"
#include "PauseAllTrigger.h"
#include "ResumeAllTrigger.h"
#include "Environment.h"
#include "Parameter.h"
#include "PreloadRequest.h"
#include "Setting.h"
#include "Switch.h"
#include "SwitchState.h"
#include "Trigger.h"
#include <IImpl.h>
#include <CryString/CryPath.h>

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "PreviewTrigger.h"
	#include "Common/Logger.h"
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
struct SXMLTags final
{
	static char const* const szPlatform;
};

#if CRY_PLATFORM_WINDOWS
char const* const SXMLTags::szPlatform = "pc";
#elif CRY_PLATFORM_DURANGO
char const* const SXMLTags::szPlatform = "xboxone";
#elif CRY_PLATFORM_ORBIS
char const* const SXMLTags::szPlatform = "ps4";
#elif CRY_PLATFORM_MAC
char const* const SXMLTags::szPlatform = "mac";
#elif CRY_PLATFORM_LINUX
char const* const SXMLTags::szPlatform = "linux";
#elif CRY_PLATFORM_IOS
char const* const SXMLTags::szPlatform = "ios";
#elif CRY_PLATFORM_ANDROID
char const* const SXMLTags::szPlatform = "linux";
#else
	#error "Undefined platform."
#endif

//////////////////////////////////////////////////////////////////////////
void ParseSystemDataFile(char const* const szFolderPath, SPoolSizes& poolSizes, bool const isLevelSpecific)
{
	CryFixedStringT<MaxFilePathLength> rootFolderPath(szFolderPath);
	rootFolderPath.TrimRight(R"(/\)");
	CryFixedStringT<MaxFilePathLength + MaxFileNameLength> search(rootFolderPath + "/*.xml");
	_finddata_t fd;
	intptr_t const handle = gEnv->pCryPak->FindFirst(search.c_str(), &fd);

	if (handle != -1)
	{
		CryFixedStringT<MaxFilePathLength + MaxFileNameLength> fileName;

		do
		{
			fileName = rootFolderPath.c_str();
			fileName += "/";
			fileName += fd.name;

			XmlNodeRef const pRootNode(GetISystem()->LoadXmlFromFile(fileName));

			if (pRootNode != nullptr)
			{
				if (_stricmp(pRootNode->getTag(), g_szRootNodeTag) == 0)
				{
					uint16 numTriggers = 0;
					pRootNode->getAttr(g_szNumTriggersAttribute, numTriggers);
					poolSizes.triggers += numTriggers;

					uint16 numParameters = 0;
					pRootNode->getAttr(g_szNumParametersAttribute, numParameters);
					poolSizes.parameters += numParameters;

					uint16 numSwitches = 0;
					pRootNode->getAttr(g_szNumSwitchesAttribute, numSwitches);
					poolSizes.switches += numSwitches;

					uint16 numStates = 0;
					pRootNode->getAttr(g_szNumStatesAttribute, numStates);
					poolSizes.states += numStates;

					uint16 numEnvironments = 0;
					pRootNode->getAttr(g_szNumEnvironmentsAttribute, numEnvironments);
					poolSizes.environments += numEnvironments;

					uint16 numPreloads = 0;
					pRootNode->getAttr(g_szNumPreloadsAttribute, numPreloads);
					poolSizes.preloads += numPreloads;

					uint16 numSettings = 0;
					pRootNode->getAttr(g_szNumSettingsAttribute, numSettings);
					poolSizes.settings += numSettings;

					uint16 numFiles = 0;
					pRootNode->getAttr(g_szNumFilesAttribute, numFiles);
					poolSizes.files += numFiles;
				}

				XmlNodeRef const pImplDataNode = pRootNode->findChild(g_szImplDataNodeTag);

				if (pImplDataNode != nullptr)
				{
					g_pIImpl->SetLibraryData(pImplDataNode, isLevelSpecific);
				}
			}
		}
		while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

		gEnv->pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void ParseLevelSpecificSystemData(char const* const szFolderPath, SPoolSizes& poolSizes)
{
	CryFixedStringT<MaxFilePathLength> levelsFolderPath(szFolderPath);
	levelsFolderPath += "levels/";

	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(levelsFolderPath + "*", &fd);

	if (handle != -1)
	{
		do
		{
			if (fd.attrib & _A_SUBDIR)
			{
				char const* const szName = fd.name;

				if ((_stricmp(szName, ".") != 0) && (_stricmp(szName, "..") != 0))
				{
					char const* const szSubFolderName = levelsFolderPath + szName;
					SPoolSizes levelPoolSizes;

					ParseSystemDataFile(szSubFolderName, levelPoolSizes, true);

					poolSizes.triggers = std::max(poolSizes.triggers, levelPoolSizes.triggers);
					poolSizes.parameters = std::max(poolSizes.parameters, levelPoolSizes.parameters);
					poolSizes.switches = std::max(poolSizes.switches, levelPoolSizes.switches);
					poolSizes.states = std::max(poolSizes.states, levelPoolSizes.states);
					poolSizes.environments = std::max(poolSizes.environments, levelPoolSizes.environments);
					poolSizes.preloads = std::max(poolSizes.preloads, levelPoolSizes.preloads);
					poolSizes.settings = std::max(poolSizes.settings, levelPoolSizes.settings);
					poolSizes.files = std::max(poolSizes.files, levelPoolSizes.files);
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseSystemData()
{
	ZeroStruct(g_poolSizes);

	g_pIImpl->OnBeforeLibraryDataChanged();
	g_pIImpl->GetInfo(g_implInfo);
	g_configPath = CRY_AUDIO_DATA_ROOT "/";
	g_configPath += (g_implInfo.folderName + "/" + g_szConfigFolderName + "/").c_str();

	ParseSystemDataFile(g_configPath.c_str(), g_poolSizes, false);

	// For level specific controls, we take the highest amount of any scope,
	// to avoid reallocating when the scope changes.
	SPoolSizes maxLevelPoolSizes;
	ParseLevelSpecificSystemData(g_configPath.c_str(), maxLevelPoolSizes);

	g_poolSizes.triggers += maxLevelPoolSizes.triggers;
	g_poolSizes.parameters += maxLevelPoolSizes.parameters;
	g_poolSizes.switches += maxLevelPoolSizes.switches;
	g_poolSizes.states += maxLevelPoolSizes.states;
	g_poolSizes.environments += maxLevelPoolSizes.environments;
	g_poolSizes.preloads += maxLevelPoolSizes.preloads;
	g_poolSizes.settings += maxLevelPoolSizes.settings;
	g_poolSizes.files += maxLevelPoolSizes.files;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	// Need to set pool sizes to at least 1, because there could be files that don't contain the
	// counts yet, which could result in asserts of the pool object.
	g_poolSizes.triggers = std::max<uint16>(1, g_poolSizes.triggers);
	g_poolSizes.parameters = std::max<uint16>(1, g_poolSizes.parameters);
	g_poolSizes.switches = std::max<uint16>(1, g_poolSizes.switches);
	g_poolSizes.states = std::max<uint16>(1, g_poolSizes.states);
	g_poolSizes.environments = std::max<uint16>(1, g_poolSizes.environments);
	g_poolSizes.preloads = std::max<uint16>(1, g_poolSizes.preloads);
	g_poolSizes.settings = std::max<uint16>(1, g_poolSizes.settings);
	g_poolSizes.files = std::max<uint16>(1, g_poolSizes.files);

	g_pIImpl->OnAfterLibraryDataChanged();
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseControlsData(char const* const szFolderPath, EDataScope const dataScope)
{
	CryFixedStringT<MaxFilePathLength> sRootFolderPath(szFolderPath);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	if (g_pIImpl != nullptr)
	{
		sRootFolderPath.TrimRight(R"(/\)");
		CryFixedStringT<MaxFilePathLength + MaxFileNameLength> sSearch(sRootFolderPath + "/*.xml");
		_finddata_t fd;
		intptr_t const handle = gEnv->pCryPak->FindFirst(sSearch.c_str(), &fd);

		if (handle != -1)
		{
			CryFixedStringT<MaxFilePathLength + MaxFileNameLength> sFileName;

			do
			{
				sFileName = sRootFolderPath.c_str();
				sFileName += "/";
				sFileName += fd.name;

				XmlNodeRef const pRootNode(GetISystem()->LoadXmlFromFile(sFileName));

				if (pRootNode)
				{
					if (_stricmp(pRootNode->getTag(), g_szRootNodeTag) == 0)
					{
						LibraryId const libraryId = StringToId(pRootNode->getAttr(g_szNameAttribute));

						if (libraryId == DefaultLibraryId)
						{
							ParseDefaultControlsFile(pRootNode);
						}
						else
						{
							ParseControlsFile(pRootNode, dataScope);
						}
					}
				}
			}
			while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

			gEnv->pCryPak->FindClose(handle);
		}
	}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	char const* szDataScope = "unknown";

	switch (dataScope)
	{
	case EDataScope::Global:
		{
			szDataScope = "Global";

			break;
		}
	case EDataScope::LevelSpecific:
		{
			szDataScope = "Level Specific";

			break;
		}
	default:
		{
			break;
		}
	}

	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	Cry::Audio::Log(ELogType::Comment, R"(Parsed controls data in "%s" for data scope "%s" in %.3f ms!)", szFolderPath, szDataScope, duration);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseControlsFile(XmlNodeRef const pRootNode, EDataScope const dataScope)
{
	int const rootChildCount = pRootNode->getChildCount();

	for (int i = 0; i < rootChildCount; ++i)
	{
		XmlNodeRef const pChildNode(pRootNode->getChild(i));

		if (pChildNode != nullptr)
		{
			char const* const szChildNodeTag = pChildNode->getTag();

			if (_stricmp(szChildNodeTag, g_szTriggersNodeTag) == 0)
			{
				ParseTriggers(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, g_szParametersNodeTag) == 0)
			{
				ParseParameters(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, g_szSwitchesNodeTag) == 0)
			{
				ParseSwitches(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, g_szEnvironmentsNodeTag) == 0)
			{
				ParseEnvironments(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, g_szSettingsNodeTag) == 0)
			{
				ParseSettings(pChildNode, dataScope);
			}
			else if ((_stricmp(szChildNodeTag, g_szImplDataNodeTag) == 0) ||
			         (_stricmp(szChildNodeTag, g_szPreloadsNodeTag) == 0) ||
			         (_stricmp(szChildNodeTag, g_szEditorDataTag) == 0))
			{
				// These tags are valid but ignored here.
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", szChildNodeTag);
			}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseDefaultControlsFile(XmlNodeRef const pRootNode)
{
	int const rootChildCount = pRootNode->getChildCount();

	for (int i = 0; i < rootChildCount; ++i)
	{
		XmlNodeRef const pChildNode(pRootNode->getChild(i));

		if (pChildNode != nullptr)
		{
			char const* const childNodeTag = pChildNode->getTag();

			if (_stricmp(childNodeTag, g_szTriggersNodeTag) == 0)
			{
				ParseDefaultTriggers(pChildNode);
			}
			else if ((_stricmp(childNodeTag, g_szImplDataNodeTag) == 0) ||
			         (_stricmp(childNodeTag, g_szParametersNodeTag) == 0) ||
			         (_stricmp(childNodeTag, g_szSwitchesNodeTag) == 0) ||
			         (_stricmp(childNodeTag, g_szEnvironmentsNodeTag) == 0) ||
			         (_stricmp(childNodeTag, g_szPreloadsNodeTag) == 0) ||
			         (_stricmp(childNodeTag, g_szSettingsNodeTag) == 0) ||
			         (_stricmp(childNodeTag, g_szEditorDataTag) == 0))
			{
				// These tags are valid but ignored here, because no default controls of these type currently exist.
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", childNodeTag);
			}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParsePreloadsData(char const* const szFolderPath, EDataScope const dataScope)
{
	CryFixedStringT<MaxFilePathLength> rootFolderPath(szFolderPath);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	if (g_pIImpl != nullptr)
	{
		rootFolderPath.TrimRight(R"(/\)");
		CryFixedStringT<MaxFilePathLength + MaxFileNameLength> search(rootFolderPath + "/*.xml");
		_finddata_t fd;
		intptr_t const handle = gEnv->pCryPak->FindFirst(search.c_str(), &fd);

		if (handle != -1)
		{
			CryFixedStringT<MaxFilePathLength + MaxFileNameLength> fileName;

			do
			{
				fileName = rootFolderPath.c_str();
				fileName += "/";
				fileName += fd.name;

				XmlNodeRef const pRootNode(GetISystem()->LoadXmlFromFile(fileName));

				if (pRootNode != nullptr)
				{
					if (_stricmp(pRootNode->getTag(), g_szRootNodeTag) == 0)
					{

						uint versionNumber = 1;
						pRootNode->getAttr(g_szVersionAttribute, versionNumber);
						int const numChildren = pRootNode->getChildCount();

						for (int i = 0; i < numChildren; ++i)
						{
							XmlNodeRef const pChildNode(pRootNode->getChild(i));

							if (pChildNode != nullptr)
							{
								char const* const szChildNodeTag = pChildNode->getTag();

								if (_stricmp(szChildNodeTag, g_szPreloadsNodeTag) == 0)
								{
									size_t const lastSlashIndex = rootFolderPath.rfind("/"[0]);

									if (rootFolderPath.npos != lastSlashIndex)
									{
										CryFixedStringT<MaxFilePathLength> const folderName(rootFolderPath.substr(lastSlashIndex + 1, rootFolderPath.size()));
										ParsePreloads(pChildNode, dataScope, folderName.c_str(), versionNumber);
									}
									else
									{
										ParsePreloads(pChildNode, dataScope, nullptr, versionNumber);
									}
								}
								else if (_stricmp(szChildNodeTag, g_szImplDataNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, g_szTriggersNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, g_szParametersNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, g_szSwitchesNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, g_szEnvironmentsNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, g_szSettingsNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, g_szEditorDataTag) == 0)
								{
									// These tags are valid but ignored here.
								}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
								else
								{
									Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", szChildNodeTag);
								}
#endif            // CRY_AUDIO_USE_PRODUCTION_CODE
							}
						}
					}
				}
			}
			while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

			gEnv->pCryPak->FindClose(handle);
		}
	}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	char const* szDataScope = "unknown";

	switch (dataScope)
	{
	case EDataScope::Global:
		{
			szDataScope = "Global";

			break;
		}
	case EDataScope::LevelSpecific:
		{
			szDataScope = "Level Specific";

			break;
		}
	default:
		{
			break;
		}
	}

	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	Cry::Audio::Log(ELogType::Comment, R"(Parsed preloads data in "%s" for data scope "%s" in %.3f ms!)", szFolderPath, szDataScope, duration);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ClearControlsData(EDataScope const dataScope)
{
	if (g_pIImpl != nullptr)
	{
		if (dataScope == EDataScope::All || dataScope == EDataScope::Global)
		{
			g_loseFocusTrigger.Clear();
			g_getFocusTrigger.Clear();
			g_muteAllTrigger.Clear();
			g_unmuteAllTrigger.Clear();
			g_pauseAllTrigger.Clear();
			g_resumeAllTrigger.Clear();

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			g_previewTrigger.Clear();
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}

		TriggerLookup::iterator iterTriggers(g_triggers.begin());
		TriggerLookup::const_iterator iterTriggersEnd(g_triggers.end());

		while (iterTriggers != iterTriggersEnd)
		{
			CTrigger const* const pTrigger = iterTriggers->second;

			if ((pTrigger->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				delete pTrigger;
				iterTriggers = g_triggers.erase(iterTriggers);
				iterTriggersEnd = g_triggers.end();
				continue;
			}

			++iterTriggers;
		}

		ParameterLookup::iterator iterParameters(g_parameters.begin());
		ParameterLookup::const_iterator iterParametersEnd(g_parameters.end());

		while (iterParameters != iterParametersEnd)
		{
			CParameter const* const pParameter = iterParameters->second;

			if ((pParameter->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				delete pParameter;
				iterParameters = g_parameters.erase(iterParameters);
				iterParametersEnd = g_parameters.end();
				continue;
			}

			++iterParameters;
		}

		SwitchLookup::iterator iterSwitches(g_switches.begin());
		SwitchLookup::const_iterator iterSwitchesEnd(g_switches.end());

		while (iterSwitches != iterSwitchesEnd)
		{
			CSwitch const* const pSwitch = iterSwitches->second;

			if ((pSwitch->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				delete pSwitch;
				iterSwitches = g_switches.erase(iterSwitches);
				iterSwitchesEnd = g_switches.end();
				continue;
			}

			++iterSwitches;
		}

		EnvironmentLookup::iterator iterEnvironments(g_environments.begin());
		EnvironmentLookup::const_iterator iterEnvironmentsEnd(g_environments.end());

		while (iterEnvironments != iterEnvironmentsEnd)
		{
			CEnvironment const* const pEnvironment = iterEnvironments->second;

			if ((pEnvironment->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				delete pEnvironment;
				iterEnvironments = g_environments.erase(iterEnvironments);
				iterEnvironmentsEnd = g_environments.end();
				continue;
			}

			++iterEnvironments;
		}

		SettingLookup::iterator iterSettings(g_settings.begin());
		SettingLookup::const_iterator iterSettingsEnd(g_settings.end());

		while (iterSettings != iterSettingsEnd)
		{
			CSetting const* const pSetting = iterSettings->second;

			if ((pSetting->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				delete pSetting;
				iterSettings = g_settings.erase(iterSettings);
				iterSettingsEnd = g_settings.end();
				continue;
			}

			++iterSettings;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParsePreloads(XmlNodeRef const pPreloadDataRoot, EDataScope const dataScope, char const* const szFolderName, uint const version)
{
	LOADING_TIME_PROFILE_SECTION;

	int const numPreloadRequests = pPreloadDataRoot->getChildCount();

	for (int i = 0; i < numPreloadRequests; ++i)
	{
		XmlNodeRef const pPreloadRequestNode(pPreloadDataRoot->getChild(i));

		if (pPreloadRequestNode && _stricmp(pPreloadRequestNode->getTag(), g_szPreloadRequestTag) == 0)
		{
			PreloadRequestId preloadRequestId = GlobalPreloadRequestId;
			char const* szPreloadRequestName = g_szGlobalPreloadRequestName;
			bool const isAutoLoad = (_stricmp(pPreloadRequestNode->getAttr(g_szTypeAttribute), g_szDataLoadType) == 0);

			if (!isAutoLoad)
			{
				szPreloadRequestName = pPreloadRequestNode->getAttr(g_szNameAttribute);
				preloadRequestId = static_cast<PreloadRequestId>(StringToId(szPreloadRequestName));
			}
			else if (dataScope == EDataScope::LevelSpecific)
			{
				szPreloadRequestName = szFolderName;
				preloadRequestId = static_cast<PreloadRequestId>(StringToId(szPreloadRequestName));
			}

			if (preloadRequestId != InvalidPreloadRequestId)
			{
				XmlNodeRef pFileListParentNode = nullptr;
				int const platformCount = pPreloadRequestNode->getChildCount();

				for (int j = 0; j < platformCount; ++j)
				{
					XmlNodeRef const pPlatformNode(pPreloadRequestNode->getChild(j));

					if ((pPlatformNode != nullptr) && (_stricmp(pPlatformNode->getAttr(g_szNameAttribute), SXMLTags::szPlatform) == 0))
					{
						pFileListParentNode = pPlatformNode;
						break;
					}
				}

				if (pFileListParentNode != nullptr)
				{
					// Found the config group corresponding to the specified platform.
					int const fileCount = pFileListParentNode->getChildCount();

					CPreloadRequest::FileIds fileIds;
					fileIds.reserve(fileCount);

					for (int k = 0; k < fileCount; ++k)
					{
						FileId const id = g_fileCacheManager.TryAddFileCacheEntry(pFileListParentNode->getChild(k), dataScope, isAutoLoad);

						if (id != InvalidFileId)
						{
							fileIds.push_back(id);
						}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
						else
						{
							Cry::Audio::Log(ELogType::Warning, R"(Preload request "%s" could not create file entry from tag "%s"!)", szPreloadRequestName, pFileListParentNode->getChild(k)->getTag());
						}
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE
					}

					CPreloadRequest* pPreloadRequest = stl::find_in_map(g_preloadRequests, preloadRequestId, nullptr);

					if (pPreloadRequest == nullptr)
					{
						MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CPreloadRequest");
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
						pPreloadRequest = new CPreloadRequest(preloadRequestId, dataScope, isAutoLoad, fileIds, szPreloadRequestName);
#else
						pPreloadRequest = new CPreloadRequest(preloadRequestId, dataScope, isAutoLoad, fileIds);
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE

						if (pPreloadRequest != nullptr)
						{
							g_preloadRequests[preloadRequestId] = pPreloadRequest;
						}
						else
						{
							CryFatalError("<Audio>: Failed to allocate CPreloadRequest");
						}
					}
					else
					{
						// Add to existing preload request.
						pPreloadRequest->m_fileIds.insert(pPreloadRequest->m_fileIds.end(), fileIds.begin(), fileIds.end());
					}
				}
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Preload request "%s" already exists! Skipping this entry!)", szPreloadRequestName);
			}
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ClearPreloadsData(EDataScope const dataScope)
{
	if (g_pIImpl != nullptr)
	{
		PreloadRequestLookup::iterator iRemover = g_preloadRequests.begin();
		PreloadRequestLookup::const_iterator const iEnd = g_preloadRequests.end();

		while (iRemover != iEnd)
		{
			CPreloadRequest const* const pRequest = iRemover->second;

			if ((pRequest->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				DeletePreloadRequest(pRequest);
				g_preloadRequests.erase(iRemover++);
			}
			else
			{
				++iRemover;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseEnvironments(XmlNodeRef const pEnvironmentRoot, EDataScope const dataScope)
{
	int const numEnvironments = pEnvironmentRoot->getChildCount();

	for (int i = 0; i < numEnvironments; ++i)
	{
		XmlNodeRef const pEnvironmentNode(pEnvironmentRoot->getChild(i));

		if ((pEnvironmentNode != nullptr) && (_stricmp(pEnvironmentNode->getTag(), g_szEnvironmentTag) == 0))
		{
			char const* const szEnvironmentName = pEnvironmentNode->getAttr(g_szNameAttribute);
			auto const environmentId = static_cast<EnvironmentId const>(StringToId(szEnvironmentName));

			if ((environmentId != InvalidControlId) && (stl::find_in_map(g_environments, environmentId, nullptr) == nullptr))
			{
				//there is no entry yet with this ID in the container
				int const numConnections = pEnvironmentNode->getChildCount();

				EnvironmentConnections connections;
				connections.reserve(numConnections);

				for (int j = 0; j < numConnections; ++j)
				{
					XmlNodeRef const pEnvironmentImplNode(pEnvironmentNode->getChild(j));

					if (pEnvironmentImplNode != nullptr)
					{
						Impl::IEnvironmentConnection* const pConnection = g_pIImpl->ConstructEnvironmentConnection(pEnvironmentImplNode);

						if (pConnection != nullptr)
						{
							connections.push_back(pConnection);
						}
					}
				}

				if (!connections.empty())
				{
					if (connections.size() < numConnections)
					{
						connections.shrink_to_fit();
					}

					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CEnvironment");
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
					auto const pNewEnvironment = new CEnvironment(environmentId, dataScope, connections, szEnvironmentName);
#else
					auto const pNewEnvironment = new CEnvironment(environmentId, dataScope, connections);
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE

					if (pNewEnvironment != nullptr)
					{
						g_environments[environmentId] = pNewEnvironment;
					}
				}
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Environment "%s" already exists!)", szEnvironmentName);
			}
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseSettings(XmlNodeRef const pRoot, EDataScope const dataScope)
{
	int const numSettings = pRoot->getChildCount();

	for (int i = 0; i < numSettings; ++i)
	{
		XmlNodeRef const pSettingNode(pRoot->getChild(i));

		if (pSettingNode && _stricmp(pSettingNode->getTag(), g_szSettingTag) == 0)
		{
			char const* const szSettingName = pSettingNode->getAttr(g_szNameAttribute);
			auto const settingId = static_cast<ControlId>(StringToId(szSettingName));

			if ((settingId != InvalidControlId) && (stl::find_in_map(g_settings, settingId, nullptr) == nullptr))
			{
				XmlNodeRef pSettingImplParentNode = nullptr;
				int const numPlatforms = pSettingNode->getChildCount();

				for (int j = 0; j < numPlatforms; ++j)
				{
					XmlNodeRef const pPlatformNode(pSettingNode->getChild(j));

					if ((pPlatformNode != nullptr) && (_stricmp(pPlatformNode->getAttr(g_szNameAttribute), SXMLTags::szPlatform) == 0))
					{
						pSettingImplParentNode = pPlatformNode;
						break;
					}
				}

				if (pSettingImplParentNode != nullptr)
				{
					bool const isAutoLoad = (_stricmp(pSettingNode->getAttr(g_szTypeAttribute), g_szDataLoadType) == 0);

					int const numConnections = pSettingImplParentNode->getChildCount();
					SettingConnections connections;
					connections.reserve(numConnections);

					for (int k = 0; k < numConnections; ++k)
					{
						XmlNodeRef const pSettingImplNode(pSettingImplParentNode->getChild(k));

						if (pSettingImplNode != nullptr)
						{
							Impl::ISettingConnection* const pConnection = g_pIImpl->ConstructSettingConnection(pSettingImplNode);

							if (pConnection != nullptr)
							{
								connections.push_back(pConnection);
							}
						}
					}

					if (!connections.empty())
					{
						if (connections.size() < numConnections)
						{
							connections.shrink_to_fit();
						}

						MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CSetting");
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
						auto const pNewSetting = new CSetting(settingId, dataScope, isAutoLoad, connections, szSettingName);
#else
						auto const pNewSetting = new CSetting(settingId, dataScope, isAutoLoad, connections);
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE

						if (pNewSetting != nullptr)
						{
							g_settings[settingId] = pNewSetting;
						}
					}
				}
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Setting "%s" already exists!)", szSettingName);
			}
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseTriggers(XmlNodeRef const pXMLTriggerRoot, EDataScope const dataScope)
{
	int const numTriggers = pXMLTriggerRoot->getChildCount();

	for (int i = 0; i < numTriggers; ++i)
	{
		XmlNodeRef const pTriggerNode(pXMLTriggerRoot->getChild(i));

		if (pTriggerNode && _stricmp(pTriggerNode->getTag(), g_szTriggerTag) == 0)
		{
			char const* const szTriggerName = pTriggerNode->getAttr(g_szNameAttribute);
			auto const triggerId = static_cast<ControlId const>(StringToId(szTriggerName));

			if ((triggerId != InvalidControlId) && (stl::find_in_map(g_triggers, triggerId, nullptr) == nullptr))
			{
				int const numConnections = pTriggerNode->getChildCount();
				TriggerConnections connections;
				connections.reserve(numConnections);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				float maxRadius = 0.0f;
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE

				for (int m = 0; m < numConnections; ++m)
				{
					XmlNodeRef const pTriggerImplNode(pTriggerNode->getChild(m));

					if (pTriggerImplNode)
					{
						float radius = 0.0f;
						Impl::ITriggerConnection* const pConnection = g_pIImpl->ConstructTriggerConnection(pTriggerImplNode, radius);

						if (pConnection != nullptr)
						{
							connections.push_back(pConnection);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
							maxRadius = std::max(radius, maxRadius);
#endif          // CRY_AUDIO_USE_PRODUCTION_CODE
						}
					}
				}

				if (!connections.empty())
				{
					if (connections.size() < numConnections)
					{
						connections.shrink_to_fit();
					}

					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CTrigger");
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
					auto const pNewTrigger = new CTrigger(triggerId, dataScope, connections, maxRadius, szTriggerName);
#else
					auto const pNewTrigger = new CTrigger(triggerId, dataScope, connections);
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE

					if (pNewTrigger != nullptr)
					{
						g_triggers[triggerId] = pNewTrigger;
					}
				}
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Trigger "%s" already exists!)", szTriggerName);
			}
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseDefaultTriggers(XmlNodeRef const pXMLTriggerRoot)
{
	int const numTriggers = pXMLTriggerRoot->getChildCount();

	for (int i = 0; i < numTriggers; ++i)
	{
		XmlNodeRef const pTriggerNode(pXMLTriggerRoot->getChild(i));

		if (pTriggerNode && _stricmp(pTriggerNode->getTag(), g_szTriggerTag) == 0)
		{
			char const* const szTriggerName = pTriggerNode->getAttr(g_szNameAttribute);
			auto const triggerId = static_cast<ControlId const>(StringToId(szTriggerName));
			int const numConnections = pTriggerNode->getChildCount();
			TriggerConnections connections;
			connections.reserve(numConnections);

			for (int j = 0; j < numConnections; ++j)
			{
				XmlNodeRef const pConnectionNode(pTriggerNode->getChild(j));

				if (pConnectionNode != nullptr)
				{
					float radius = 0.0f;
					Impl::ITriggerConnection* const pConnection = g_pIImpl->ConstructTriggerConnection(pConnectionNode, radius);

					if (pConnection != nullptr)
					{
						connections.push_back(pConnection);
					}
				}
			}

			if (!connections.empty())
			{
				if (connections.size() < numConnections)
				{
					connections.shrink_to_fit();
				}

				switch (triggerId)
				{
				case g_loseFocusTriggerId:
					{
						g_loseFocusTrigger.AddConnections(connections);
						break;
					}
				case g_getFocusTriggerId:
					{
						g_getFocusTrigger.AddConnections(connections);
						break;
					}
				case g_muteAllTriggerId:
					{
						g_muteAllTrigger.AddConnections(connections);
						break;
					}
				case g_unmuteAllTriggerId:
					{
						g_unmuteAllTrigger.AddConnections(connections);
						break;
					}
				case g_pauseAllTriggerId:
					{
						g_pauseAllTrigger.AddConnections(connections);
						break;
					}
				case g_resumeAllTriggerId:
					{
						g_resumeAllTrigger.AddConnections(connections);
						break;
					}
				default:
					{
						CRY_ASSERT_MESSAGE(false, R"(The default trigger "%s" does not exist during %s)", szTriggerName, __FUNCTION__);
						break;
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseSwitches(XmlNodeRef const pXMLSwitchRoot, EDataScope const dataScope)
{
	int const numSwitches = pXMLSwitchRoot->getChildCount();

	for (int i = 0; i < numSwitches; ++i)
	{
		XmlNodeRef const pSwitchNode(pXMLSwitchRoot->getChild(i));

		if (pSwitchNode && _stricmp(pSwitchNode->getTag(), g_szSwitchTag) == 0)
		{
			char const* const szSwitchName = pSwitchNode->getAttr(g_szNameAttribute);
			auto const switchId = static_cast<ControlId const>(StringToId(szSwitchName));

			if ((switchId != InvalidControlId) && (stl::find_in_map(g_switches, switchId, nullptr) == nullptr))
			{
				MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CSwitch");
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				auto const pNewSwitch = new CSwitch(switchId, dataScope, szSwitchName);
#else
				auto const pNewSwitch = new CSwitch(switchId, dataScope);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

				int const numStates = pSwitchNode->getChildCount();

				for (int j = 0; j < numStates; ++j)
				{
					XmlNodeRef const pSwitchStateNode(pSwitchNode->getChild(j));

					if (pSwitchStateNode && _stricmp(pSwitchStateNode->getTag(), g_szStateTag) == 0)
					{
						char const* const szSwitchStateName = pSwitchStateNode->getAttr(g_szNameAttribute);
						auto const switchStateId = static_cast<SwitchStateId const>(StringToId(szSwitchStateName));

						if (switchStateId != InvalidSwitchStateId)
						{
							int const numConnections = pSwitchStateNode->getChildCount();
							SwitchStateConnections connections;
							connections.reserve(numConnections);

							for (int k = 0; k < numConnections; ++k)
							{
								XmlNodeRef const pStateImplNode(pSwitchStateNode->getChild(k));

								if (pStateImplNode != nullptr)
								{
									Impl::ISwitchStateConnection* const pConnection = g_pIImpl->ConstructSwitchStateConnection(pStateImplNode);

									if (pConnection != nullptr)
									{
										connections.push_back(pConnection);
									}
								}
							}

							if (!connections.empty())
							{
								if (connections.size() < numConnections)
								{
									connections.shrink_to_fit();
								}

								MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CSwitchState");
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
								auto const pNewState = new CSwitchState(switchId, switchStateId, connections, szSwitchStateName);
#else
								auto const pNewState = new CSwitchState(switchId, switchStateId, connections);
#endif            // CRY_AUDIO_USE_PRODUCTION_CODE

								pNewSwitch->AddState(switchStateId, pNewState);
							}
						}
					}
				}

				g_switches[switchId] = pNewSwitch;
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Switch "%s" already exists!)", szSwitchName);
			}
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseParameters(XmlNodeRef const pXMLParameterRoot, EDataScope const dataScope)
{
	int const numParameters = pXMLParameterRoot->getChildCount();

	for (int i = 0; i < numParameters; ++i)
	{
		XmlNodeRef const pParameterNode(pXMLParameterRoot->getChild(i));

		if (pParameterNode && _stricmp(pParameterNode->getTag(), g_szParameterTag) == 0)
		{
			char const* const szParameterName = pParameterNode->getAttr(g_szNameAttribute);
			auto const parameterId = static_cast<ControlId const>(StringToId(szParameterName));

			if ((parameterId != InvalidControlId) && (stl::find_in_map(g_parameters, parameterId, nullptr) == nullptr))
			{
				int const numConnections = pParameterNode->getChildCount();
				ParameterConnections connections;
				connections.reserve(numConnections);

				for (int j = 0; j < numConnections; ++j)
				{
					XmlNodeRef const pParameterImplNode(pParameterNode->getChild(j));

					if (pParameterImplNode != nullptr)
					{
						Impl::IParameterConnection* const pConnection = g_pIImpl->ConstructParameterConnection(pParameterImplNode);

						if (pConnection != nullptr)
						{
							connections.push_back(pConnection);
						}
					}
				}

				if (!connections.empty())
				{
					if (connections.size() < numConnections)
					{
						connections.shrink_to_fit();
					}

					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CParameter");
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
					auto const pParameter = new CParameter(parameterId, dataScope, connections, szParameterName);
#else
					auto const pParameter = new CParameter(parameterId, dataScope, connections);
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE

					if (pParameter != nullptr)
					{
						g_parameters[parameterId] = pParameter;
					}
				}
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Parameter "%s" already exists!)", szParameterName);
			}
#endif        // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::DeletePreloadRequest(CPreloadRequest const* const pPreloadRequest)
{
	if (pPreloadRequest != nullptr)
	{
		EDataScope const dataScope = pPreloadRequest->GetDataScope();

		for (auto const fileId : pPreloadRequest->m_fileIds)
		{
			g_fileCacheManager.TryRemoveFileCacheEntry(fileId, dataScope);
		}

		delete pPreloadRequest;
	}
}
} // namespace CryAudio
