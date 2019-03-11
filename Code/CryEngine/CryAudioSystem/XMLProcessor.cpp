// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "XMLProcessor.h"
#include "CVars.h"
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

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	#include "PreviewTrigger.h"
	#include "Common/Logger.h"
#endif // CRY_AUDIO_USE_DEBUG_CODE

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
bool ParseSystemDataFile(char const* const szFolderPath, SPoolSizes& poolSizes, ContextId const contextId)
{
	bool parsedValidLibrary = false;

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

					XmlNodeRef const pImplDataNode = pRootNode->findChild(g_szImplDataNodeTag);

					if (pImplDataNode != nullptr)
					{
						g_pIImpl->SetLibraryData(pImplDataNode, contextId);
					}

					parsedValidLibrary = true;
				}
			}
		}
		while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

		gEnv->pCryPak->FindClose(handle);
	}

	return parsedValidLibrary;
}

//////////////////////////////////////////////////////////////////////////
void ParseContextSystemData(char const* const szFolderPath, SPoolSizes& poolSizes)
{
	CryFixedStringT<MaxFilePathLength> contextFolderPath(szFolderPath);
	contextFolderPath += g_szContextsFolderName;
	contextFolderPath += "/";

	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(contextFolderPath + "*", &fd);

	if (handle != -1)
	{
		do
		{
			if (fd.attrib & _A_SUBDIR)
			{
				char const* const szName = fd.name;

				if ((_stricmp(szName, ".") != 0) && (_stricmp(szName, "..") != 0))
				{
					char const* const szSubFolderName = contextFolderPath + szName;
					ContextId const contextId = StringToId(szName);
					SPoolSizes contextPoolSizes;

					if (ParseSystemDataFile(szSubFolderName, contextPoolSizes, contextId))
					{
						g_registeredContexts[contextId] = szName;
					}

					if (g_cvars.m_poolAllocationMode <= 0)
					{
						poolSizes.triggers += contextPoolSizes.triggers;
						poolSizes.parameters += contextPoolSizes.parameters;
						poolSizes.switches += contextPoolSizes.switches;
						poolSizes.states += contextPoolSizes.states;
						poolSizes.environments += contextPoolSizes.environments;
						poolSizes.preloads += contextPoolSizes.preloads;
						poolSizes.settings += contextPoolSizes.settings;
						poolSizes.files += contextPoolSizes.files;
					}
					else
					{
						poolSizes.triggers = std::max(poolSizes.triggers, contextPoolSizes.triggers);
						poolSizes.parameters = std::max(poolSizes.parameters, contextPoolSizes.parameters);
						poolSizes.switches = std::max(poolSizes.switches, contextPoolSizes.switches);
						poolSizes.states = std::max(poolSizes.states, contextPoolSizes.states);
						poolSizes.environments = std::max(poolSizes.environments, contextPoolSizes.environments);
						poolSizes.preloads = std::max(poolSizes.preloads, contextPoolSizes.preloads);
						poolSizes.settings = std::max(poolSizes.settings, contextPoolSizes.settings);
						poolSizes.files = std::max(poolSizes.files, contextPoolSizes.files);
					}
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
	g_registeredContexts.clear();

	g_pIImpl->OnBeforeLibraryDataChanged();
	g_pIImpl->GetInfo(g_implInfo);
	g_configPath = CRY_AUDIO_DATA_ROOT "/";
	g_configPath += (g_implInfo.folderName + "/" + g_szConfigFolderName + "/").c_str();

	ParseSystemDataFile(g_configPath.c_str(), g_poolSizes, GlobalContextId);

	SPoolSizes contextPoolSizes;
	ParseContextSystemData(g_configPath.c_str(), contextPoolSizes);

	g_poolSizes.triggers += contextPoolSizes.triggers;
	g_poolSizes.parameters += contextPoolSizes.parameters;
	g_poolSizes.switches += contextPoolSizes.switches;
	g_poolSizes.states += contextPoolSizes.states;
	g_poolSizes.environments += contextPoolSizes.environments;
	g_poolSizes.preloads += contextPoolSizes.preloads;
	g_poolSizes.settings += contextPoolSizes.settings;
	g_poolSizes.files += contextPoolSizes.files;

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif // CRY_AUDIO_USE_DEBUG_CODE

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

	g_pIImpl->OnAfterLibraryDataChanged(g_cvars.m_poolAllocationMode);
}

//////////////////////////////////////////////////////////////////////////
bool CXMLProcessor::ParseControlsData(char const* const szFolderPath, ContextId const contextId, char const* const szContextName)
{
	bool contextExists = false;

	CryFixedStringT<MaxFilePathLength> sRootFolderPath(szFolderPath);

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // CRY_AUDIO_USE_DEBUG_CODE

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
							ParseControlsFile(pRootNode, contextId);
						}

						contextExists = true;

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
						g_activeContexts[contextId] = szContextName;
#endif        // CRY_AUDIO_USE_DEBUG_CODE
					}
				}
			}
			while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

			gEnv->pCryPak->FindClose(handle);
		}
	}

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	Cry::Audio::Log(ELogType::Comment, R"(Parsed controls data in "%s" for context "%s" in %.3f ms!)", szFolderPath, szContextName, duration);
#endif // CRY_AUDIO_USE_DEBUG_CODE

	return contextExists;
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseControlsFile(XmlNodeRef const pRootNode, ContextId const contextId)
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
				ParseTriggers(pChildNode, contextId);
			}
			else if (_stricmp(szChildNodeTag, g_szParametersNodeTag) == 0)
			{
				ParseParameters(pChildNode, contextId);
			}
			else if (_stricmp(szChildNodeTag, g_szSwitchesNodeTag) == 0)
			{
				ParseSwitches(pChildNode, contextId);
			}
			else if (_stricmp(szChildNodeTag, g_szEnvironmentsNodeTag) == 0)
			{
				ParseEnvironments(pChildNode, contextId);
			}
			else if (_stricmp(szChildNodeTag, g_szSettingsNodeTag) == 0)
			{
				ParseSettings(pChildNode, contextId);
			}
			else if ((_stricmp(szChildNodeTag, g_szImplDataNodeTag) == 0) ||
			         (_stricmp(szChildNodeTag, g_szPreloadsNodeTag) == 0) ||
			         (_stricmp(szChildNodeTag, g_szEditorDataTag) == 0))
			{
				// These tags are valid but ignored here.
			}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", szChildNodeTag);
			}
#endif  // CRY_AUDIO_USE_DEBUG_CODE
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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", childNodeTag);
			}
#endif  // CRY_AUDIO_USE_DEBUG_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParsePreloadsData(char const* const szFolderPath, ContextId const contextId)
{
	CryFixedStringT<MaxFilePathLength> rootFolderPath(szFolderPath);

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // CRY_AUDIO_USE_DEBUG_CODE

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
										ParsePreloads(pChildNode, contextId, folderName.c_str(), versionNumber);
									}
									else
									{
										ParsePreloads(pChildNode, contextId, nullptr, versionNumber);
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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
								else
								{
									Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", szChildNodeTag);
								}
#endif            // CRY_AUDIO_USE_DEBUG_CODE
							}
						}
					}
				}
			}
			while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

			gEnv->pCryPak->FindClose(handle);
		}
	}

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	char const* const szContextName = stl::find_in_map(g_activeContexts, contextId, "unknown");
	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	Cry::Audio::Log(ELogType::Comment, R"(Parsed preloads data in "%s" for context "%s" in %.3f ms!)", szFolderPath, szContextName, duration);
#endif // CRY_AUDIO_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ClearControlsData(ContextId const contextId, bool const clearAll)
{
	if (g_pIImpl != nullptr)
	{
		if (clearAll || (contextId == GlobalContextId))
		{
			g_loseFocusTrigger.Clear();
			g_getFocusTrigger.Clear();
			g_muteAllTrigger.Clear();
			g_unmuteAllTrigger.Clear();
			g_pauseAllTrigger.Clear();
			g_resumeAllTrigger.Clear();

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			g_previewTrigger.Clear();
#endif  // CRY_AUDIO_USE_DEBUG_CODE
		}

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
		if (clearAll)
		{
			g_activeContexts.clear();
		}
		else
		{
			g_activeContexts.erase(contextId);
		}
#endif  // CRY_AUDIO_USE_DEBUG_CODE

		TriggerLookup::iterator iterTriggers(g_triggers.begin());
		TriggerLookup::const_iterator iterTriggersEnd(g_triggers.end());

		while (iterTriggers != iterTriggersEnd)
		{
			CTrigger const* const pTrigger = iterTriggers->second;

			if (clearAll || (pTrigger->GetContextId() == contextId))
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

			if (clearAll || (pParameter->GetContextId() == contextId))
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

			if (clearAll || (pSwitch->GetContextId() == contextId))
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

			if (clearAll || (pEnvironment->GetContextId() == contextId))
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

			if (clearAll || (pSetting->GetContextId() == contextId))
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
void CXMLProcessor::ParsePreloads(XmlNodeRef const pPreloadDataRoot, ContextId const contextId, char const* const szFolderName, uint const version)
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
			else if (contextId != GlobalContextId)
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
						FileId const id = g_fileCacheManager.TryAddFileCacheEntry(pFileListParentNode->getChild(k), contextId, isAutoLoad);

						if (id != InvalidFileId)
						{
							fileIds.push_back(id);
						}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
						else
						{
							Cry::Audio::Log(ELogType::Warning, R"(Preload request "%s" could not create file entry from tag "%s"!)", szPreloadRequestName, pFileListParentNode->getChild(k)->getTag());
						}
#endif        // CRY_AUDIO_USE_DEBUG_CODE
					}

					CPreloadRequest* pPreloadRequest = stl::find_in_map(g_preloadRequests, preloadRequestId, nullptr);

					if (pPreloadRequest == nullptr)
					{
						MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CPreloadRequest");
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
						pPreloadRequest = new CPreloadRequest(preloadRequestId, contextId, isAutoLoad, fileIds, szPreloadRequestName);
#else
						pPreloadRequest = new CPreloadRequest(preloadRequestId, contextId, isAutoLoad, fileIds);
#endif        // CRY_AUDIO_USE_DEBUG_CODE

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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Preload request "%s" already exists! Skipping this entry!)", szPreloadRequestName);
			}
#endif        // CRY_AUDIO_USE_DEBUG_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ClearPreloadsData(ContextId const contextId, bool const clearAll)
{
	if (g_pIImpl != nullptr)
	{
		PreloadRequestLookup::iterator iter = g_preloadRequests.begin();
		PreloadRequestLookup::const_iterator const iterEnd = g_preloadRequests.end();

		while (iter != iterEnd)
		{
			CPreloadRequest const* const pRequest = iter->second;

			if (clearAll || (pRequest->GetContextId() == contextId))
			{
				DeletePreloadRequest(pRequest);
				g_preloadRequests.erase(iter++);
			}
			else
			{
				++iter;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseEnvironments(XmlNodeRef const pEnvironmentRoot, ContextId const contextId)
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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
					auto const pNewEnvironment = new CEnvironment(environmentId, contextId, connections, szEnvironmentName);
#else
					auto const pNewEnvironment = new CEnvironment(environmentId, contextId, connections);
#endif      // CRY_AUDIO_USE_DEBUG_CODE

					if (pNewEnvironment != nullptr)
					{
						g_environments[environmentId] = pNewEnvironment;
					}
				}
			}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Environment "%s" already exists!)", szEnvironmentName);
			}
#endif      // CRY_AUDIO_USE_DEBUG_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseSettings(XmlNodeRef const pRoot, ContextId const contextId)
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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
						auto const pNewSetting = new CSetting(settingId, contextId, isAutoLoad, connections, szSettingName);
#else
						auto const pNewSetting = new CSetting(settingId, contextId, isAutoLoad, connections);
#endif        // CRY_AUDIO_USE_DEBUG_CODE

						if (pNewSetting != nullptr)
						{
							g_settings[settingId] = pNewSetting;
						}
					}
				}
			}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Setting "%s" already exists!)", szSettingName);
			}
#endif      // CRY_AUDIO_USE_DEBUG_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseTriggers(XmlNodeRef const pXMLTriggerRoot, ContextId const contextId)
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

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
				float maxRadius = 0.0f;
#endif        // CRY_AUDIO_USE_DEBUG_CODE

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

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
							maxRadius = std::max(radius, maxRadius);
#endif          // CRY_AUDIO_USE_DEBUG_CODE
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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
					auto const pNewTrigger = new CTrigger(triggerId, contextId, connections, maxRadius, szTriggerName);
#else
					auto const pNewTrigger = new CTrigger(triggerId, contextId, connections);
#endif        // CRY_AUDIO_USE_DEBUG_CODE

					if (pNewTrigger != nullptr)
					{
						g_triggers[triggerId] = pNewTrigger;
					}
				}
			}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Trigger "%s" already exists!)", szTriggerName);
			}
#endif        // CRY_AUDIO_USE_DEBUG_CODE
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
void CXMLProcessor::ParseSwitches(XmlNodeRef const pXMLSwitchRoot, ContextId const contextId)
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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
				auto const pNewSwitch = new CSwitch(switchId, contextId, szSwitchName);
#else
				auto const pNewSwitch = new CSwitch(switchId, contextId);
#endif    // CRY_AUDIO_USE_DEBUG_CODE

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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
								auto const pNewState = new CSwitchState(switchId, switchStateId, connections, szSwitchStateName);
#else
								auto const pNewState = new CSwitchState(switchId, switchStateId, connections);
#endif            // CRY_AUDIO_USE_DEBUG_CODE

								pNewSwitch->AddState(switchStateId, pNewState);
							}
						}
					}
				}

				g_switches[switchId] = pNewSwitch;
			}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Switch "%s" already exists!)", szSwitchName);
			}
#endif        // CRY_AUDIO_USE_DEBUG_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::ParseParameters(XmlNodeRef const pXMLParameterRoot, ContextId const contextId)
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
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
					auto const pParameter = new CParameter(parameterId, contextId, connections, szParameterName);
#else
					auto const pParameter = new CParameter(parameterId, contextId, connections);
#endif        // CRY_AUDIO_USE_DEBUG_CODE

					if (pParameter != nullptr)
					{
						g_parameters[parameterId] = pParameter;
					}
				}
			}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Parameter "%s" already exists!)", szParameterName);
			}
#endif        // CRY_AUDIO_USE_DEBUG_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXMLProcessor::DeletePreloadRequest(CPreloadRequest const* const pPreloadRequest)
{
	if (pPreloadRequest != nullptr)
	{
		ContextId const contextId = pPreloadRequest->GetContextId();

		for (auto const fileId : pPreloadRequest->m_fileIds)
		{
			g_fileCacheManager.TryRemoveFileCacheEntry(fileId, contextId);
		}

		delete pPreloadRequest;
	}
}
} // namespace CryAudio
