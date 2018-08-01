// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioXMLProcessor.h"
#include "InternalEntities.h"
#include "Common/Logger.h"
#include "Common.h"
#include <IAudioImpl.h>
#include <CryString/CryPath.h>

namespace CryAudio
{
#define AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED 100 // IDs below that value are used for the CATLTriggerImpl_Internal

//////////////////////////////////////////////////////////////////////////
CAudioXMLProcessor::CAudioXMLProcessor()
	: m_triggerImplIdCounter(AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED)
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseControlsData(char const* const szFolderPath, EDataScope const dataScope)
{
	CryFixedStringT<MaxFilePathLength> sRootFolderPath(szFolderPath);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	if (g_pIImpl != nullptr)
	{
		sRootFolderPath.TrimRight(R"(/\)");
		CryFixedStringT<MaxFilePathLength + MaxFileNameLength> sSearch(sRootFolderPath + "/*.xml");
		_finddata_t fd;
		intptr_t handle = gEnv->pCryPak->FindFirst(sSearch.c_str(), &fd);

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
					if (_stricmp(pRootNode->getTag(), s_szRootNodeTag) == 0)
					{
						LibraryId const libraryId = StringToId(pRootNode->getAttr(s_szNameAttribute));

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

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// This is a safety precaution in case no default_controls file exists.
	ParameterConnections const parameterConnections;

	if (g_pAbsoluteVelocityParameter == nullptr)
	{
		g_pAbsoluteVelocityParameter = new CAbsoluteVelocityParameter(parameterConnections);
	}

	if (g_pRelativeVelocityParameter == nullptr)
	{
		g_pRelativeVelocityParameter = new CRelativeVelocityParameter(parameterConnections);
	}

	TriggerConnections const triggerConnections;

	if (g_pLoseFocusTrigger == nullptr)
	{
		g_pLoseFocusTrigger = new CLoseFocusTrigger(triggerConnections);
	}

	if (g_pGetFocusTrigger == nullptr)
	{
		g_pGetFocusTrigger = new CGetFocusTrigger(triggerConnections);
	}

	if (g_pMuteAllTrigger == nullptr)
	{
		g_pMuteAllTrigger = new CMuteAllTrigger(triggerConnections);
	}

	if (g_pUnmuteAllTrigger == nullptr)
	{
		g_pUnmuteAllTrigger = new CUnmuteAllTrigger(triggerConnections);
	}

	if (g_pPauseAllTrigger == nullptr)
	{
		g_pPauseAllTrigger = new CPauseAllTrigger(triggerConnections);
	}

	if (g_pResumeAllTrigger == nullptr)
	{
		g_pResumeAllTrigger = new CResumeAllTrigger(triggerConnections);
	}

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
	}

	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	Cry::Audio::Log(ELogType::Comment, R"(Parsed controls data in "%s" for data scope "%s" in %.3f ms!)", szFolderPath, szDataScope, duration);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseControlsFile(XmlNodeRef const pRootNode, EDataScope const dataScope)
{
	int const rootChildCount = pRootNode->getChildCount();

	for (int i = 0; i < rootChildCount; ++i)
	{
		XmlNodeRef const pChildNode(pRootNode->getChild(i));

		if (pChildNode != nullptr)
		{
			char const* const szChildNodeTag = pChildNode->getTag();

			if (_stricmp(szChildNodeTag, s_szTriggersNodeTag) == 0)
			{
				ParseTriggers(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, s_szParametersNodeTag) == 0)
			{
				ParseParameters(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, s_szSwitchesNodeTag) == 0)
			{
				ParseSwitches(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, s_szEnvironmentsNodeTag) == 0)
			{
				ParseEnvironments(pChildNode, dataScope);
			}
			else if (_stricmp(szChildNodeTag, s_szPreloadsNodeTag) == 0 ||
			         _stricmp(szChildNodeTag, s_szEditorDataTag) == 0)
			{
				// This tag is valid but ignored here.
			}
			else
			{
				Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", szChildNodeTag);
				CRY_ASSERT(false);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseDefaultControlsFile(XmlNodeRef const pRootNode)
{
	int const rootChildCount = pRootNode->getChildCount();

	for (int i = 0; i < rootChildCount; ++i)
	{
		XmlNodeRef const pChildNode(pRootNode->getChild(i));

		if (pChildNode != nullptr)
		{
			char const* const childNodeTag = pChildNode->getTag();

			if (_stricmp(childNodeTag, s_szTriggersNodeTag) == 0)
			{
				ParseDefaultTriggers(pChildNode);
			}
			else if (_stricmp(childNodeTag, s_szParametersNodeTag) == 0)
			{
				ParseDefaultParameters(pChildNode);
			}
			else if ((_stricmp(childNodeTag, s_szSwitchesNodeTag) == 0) ||
			         (_stricmp(childNodeTag, s_szEnvironmentsNodeTag) == 0) ||
			         (_stricmp(childNodeTag, s_szPreloadsNodeTag) == 0) ||
			         (_stricmp(childNodeTag, s_szEditorDataTag) == 0))
			{
				// These tags are valid but ignored here, beacuse no default controls of these type currently exist.
			}
			else
			{
				Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", childNodeTag);
				CRY_ASSERT(false);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParsePreloadsData(char const* const szFolderPath, EDataScope const dataScope)
{
	CryFixedStringT<MaxFilePathLength> rootFolderPath(szFolderPath);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

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
					if (_stricmp(pRootNode->getTag(), s_szRootNodeTag) == 0)
					{

						uint versionNumber = 1;
						pRootNode->getAttr(s_szVersionAttribute, versionNumber);
						int const numChildren = pRootNode->getChildCount();

						for (int i = 0; i < numChildren; ++i)
						{
							XmlNodeRef const pChildNode(pRootNode->getChild(i));

							if (pChildNode != nullptr)
							{
								char const* const szChildNodeTag = pChildNode->getTag();

								if (_stricmp(szChildNodeTag, s_szPreloadsNodeTag) == 0)
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
								else if (_stricmp(szChildNodeTag, s_szTriggersNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, s_szParametersNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, s_szSwitchesNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, s_szEnvironmentsNodeTag) == 0 ||
								         _stricmp(szChildNodeTag, s_szEditorDataTag) == 0)
								{
									// These tags are valid but ignored here.
								}
								else
								{
									Cry::Audio::Log(ELogType::Warning, "Unknown AudioSystemData node: %s", szChildNodeTag);
								}
							}
						}
					}
				}
			}
			while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

			gEnv->pCryPak->FindClose(handle);
		}
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
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
	}

	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	Cry::Audio::Log(ELogType::Comment, R"(Parsed preloads data in "%s" for data scope "%s" in %.3f ms!)", szFolderPath, szDataScope, duration);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ClearControlsData(EDataScope const dataScope)
{
	if (g_pIImpl != nullptr)
	{
		if (dataScope == EDataScope::All || dataScope == EDataScope::Global)
		{
			delete g_pAbsoluteVelocityParameter;
			g_pAbsoluteVelocityParameter = nullptr;

			delete g_pRelativeVelocityParameter;
			g_pRelativeVelocityParameter = nullptr;

			delete g_pLoseFocusTrigger;
			g_pLoseFocusTrigger = nullptr;

			delete g_pGetFocusTrigger;
			g_pGetFocusTrigger = nullptr;

			delete g_pMuteAllTrigger;
			g_pMuteAllTrigger = nullptr;

			delete g_pUnmuteAllTrigger;
			g_pUnmuteAllTrigger = nullptr;

			delete g_pPauseAllTrigger;
			g_pPauseAllTrigger = nullptr;

			delete g_pResumeAllTrigger;
			g_pResumeAllTrigger = nullptr;
		}

		AudioTriggerLookup::iterator iterTriggers(g_triggers.begin());
		AudioTriggerLookup::const_iterator iterTriggersEnd(g_triggers.end());

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

		AudioParameterLookup::iterator iterParameters(g_parameters.begin());
		AudioParameterLookup::const_iterator iterParametersEnd(g_parameters.end());

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

		AudioSwitchLookup::iterator iterSwitches(g_switches.begin());
		AudioSwitchLookup::const_iterator iterSwitchesEnd(g_switches.end());

		while (iterSwitches != iterSwitchesEnd)
		{
			CATLSwitch const* const pSwitch = iterSwitches->second;

			if ((pSwitch->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				DeleteSwitch(pSwitch);
				iterSwitches = g_switches.erase(iterSwitches);
				iterSwitchesEnd = g_switches.end();
				continue;
			}

			++iterSwitches;
		}

		AudioEnvironmentLookup::iterator iterEnvironments(g_environments.begin());
		AudioEnvironmentLookup::const_iterator iterEnvironmentsEnd(g_environments.end());

		while (iterEnvironments != iterEnvironmentsEnd)
		{
			CATLAudioEnvironment const* const pEnvironment = iterEnvironments->second;

			if ((pEnvironment->GetDataScope() == dataScope) || dataScope == EDataScope::All)
			{
				DeleteEnvironment(pEnvironment);
				iterEnvironments = g_environments.erase(iterEnvironments);
				iterEnvironmentsEnd = g_environments.end();
				continue;
			}

			++iterEnvironments;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParsePreloads(XmlNodeRef const pPreloadDataRoot, EDataScope const dataScope, char const* const szFolderName, uint const version)
{
	LOADING_TIME_PROFILE_SECTION;

	int const numPreloadRequests = pPreloadDataRoot->getChildCount();

	for (int i = 0; i < numPreloadRequests; ++i)
	{
		XmlNodeRef const pPreloadRequestNode(pPreloadDataRoot->getChild(i));

		if (pPreloadRequestNode && _stricmp(pPreloadRequestNode->getTag(), s_szPreloadRequestTag) == 0)
		{
			PreloadRequestId preloadRequestId = GlobalPreloadRequestId;
			char const* szPreloadRequestName = s_szGlobalPreloadRequestName;
			bool const isAutoLoad = (_stricmp(pPreloadRequestNode->getAttr(s_szTypeAttribute), s_szDataLoadType) == 0);

			if (!isAutoLoad)
			{
				szPreloadRequestName = pPreloadRequestNode->getAttr(s_szNameAttribute);
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

					if ((pPlatformNode != nullptr) && (_stricmp(pPlatformNode->getAttr(s_szNameAttribute), SATLXMLTags::szPlatform) == 0))
					{
						pFileListParentNode = pPlatformNode;
						break;
					}
				}

				if (pFileListParentNode != nullptr)
				{
					// Found the config group corresponding to the specified platform.
					int const fileCount = pFileListParentNode->getChildCount();

					CATLPreloadRequest::FileEntryIds cFileEntryIDs;
					cFileEntryIDs.reserve(fileCount);

					for (int k = 0; k < fileCount; ++k)
					{
						FileEntryId const id = g_fileCacheManager.TryAddFileCacheEntry(pFileListParentNode->getChild(k), dataScope, isAutoLoad);

						if (id != InvalidFileEntryId)
						{
							cFileEntryIDs.push_back(id);
						}
						else
						{
							Cry::Audio::Log(ELogType::Warning, R"(Preload request "%s" could not create file entry from tag "%s"!)", szPreloadRequestName, pFileListParentNode->getChild(k)->getTag());
						}
					}

					CATLPreloadRequest* pPreloadRequest = stl::find_in_map(g_preloadRequests, preloadRequestId, nullptr);

					if (pPreloadRequest == nullptr)
					{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
						pPreloadRequest = new CATLPreloadRequest(preloadRequestId, dataScope, isAutoLoad, cFileEntryIDs, szPreloadRequestName);
#else
						pPreloadRequest = new CATLPreloadRequest(preloadRequestId, dataScope, isAutoLoad, cFileEntryIDs);
#endif        // INCLUDE_AUDIO_PRODUCTION_CODE

						if (pPreloadRequest != nullptr)
						{
							g_preloadRequests[preloadRequestId] = pPreloadRequest;
						}
						else
						{
							CryFatalError("<Audio>: Failed to allocate CATLPreloadRequest");
						}
					}
					else
					{
						// Add to existing preload request.
						pPreloadRequest->m_fileEntryIds.insert(pPreloadRequest->m_fileEntryIds.end(), cFileEntryIDs.begin(), cFileEntryIDs.end());
					}
				}
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Preload request "%s" already exists! Skipping this entry!)", szPreloadRequestName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ClearPreloadsData(EDataScope const dataScope)
{
	if (g_pIImpl != nullptr)
	{
		AudioPreloadRequestLookup::iterator iRemover = g_preloadRequests.begin();
		AudioPreloadRequestLookup::const_iterator const iEnd = g_preloadRequests.end();

		while (iRemover != iEnd)
		{
			CATLPreloadRequest const* const pRequest = iRemover->second;

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
void CAudioXMLProcessor::ParseEnvironments(XmlNodeRef const pAudioEnvironmentRoot, EDataScope const dataScope)
{
	int const numEnvironments = pAudioEnvironmentRoot->getChildCount();

	for (int i = 0; i < numEnvironments; ++i)
	{
		XmlNodeRef const pEnvironmentNode(pAudioEnvironmentRoot->getChild(i));

		if ((pEnvironmentNode != nullptr) && (_stricmp(pEnvironmentNode->getTag(), s_szEnvironmentTag) == 0))
		{
			char const* const szEnvironmentName = pEnvironmentNode->getAttr(s_szNameAttribute);
			auto const environmentId = static_cast<EnvironmentId const>(StringToId(szEnvironmentName));

			if ((environmentId != InvalidControlId) && (stl::find_in_map(g_environments, environmentId, nullptr) == nullptr))
			{
				//there is no entry yet with this ID in the container
				int const numConnections = pEnvironmentNode->getChildCount();

				CATLAudioEnvironment::ImplPtrVec connections;
				connections.reserve(numConnections);

				for (int j = 0; j < numConnections; ++j)
				{
					XmlNodeRef const pEnvironmentImplNode(pEnvironmentNode->getChild(j));

					if (pEnvironmentImplNode != nullptr)
					{
						Impl::IEnvironment const* const pIEnvironment = g_pIImpl->ConstructEnvironment(pEnvironmentImplNode);

						if (pIEnvironment != nullptr)
						{
							CATLEnvironmentImpl const* const pEnvirnomentImpl = new CATLEnvironmentImpl(pIEnvironment);
							connections.push_back(pEnvirnomentImpl);
						}
					}
				}

				connections.shrink_to_fit();

				if (!connections.empty())
				{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
					auto const pNewEnvironment = new CATLAudioEnvironment(environmentId, dataScope, connections, szEnvironmentName);
#else
					auto const pNewEnvironment = new CATLAudioEnvironment(environmentId, dataScope, connections);
#endif      // INCLUDE_AUDIO_PRODUCTION_CODE

					if (pNewEnvironment != nullptr)
					{
						g_environments[environmentId] = pNewEnvironment;
					}
				}
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Environment "%s" already exists!)", szEnvironmentName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseTriggers(XmlNodeRef const pXMLTriggerRoot, EDataScope const dataScope)
{
	int const numTriggers = pXMLTriggerRoot->getChildCount();

	for (int i = 0; i < numTriggers; ++i)
	{
		XmlNodeRef const pTriggerNode(pXMLTriggerRoot->getChild(i));

		if (pTriggerNode && _stricmp(pTriggerNode->getTag(), s_szTriggerTag) == 0)
		{
			char const* const szTriggerName = pTriggerNode->getAttr(s_szNameAttribute);
			auto const triggerId = static_cast<ControlId const>(StringToId(szTriggerName));

			if ((triggerId != InvalidControlId) && (stl::find_in_map(g_triggers, triggerId, nullptr) == nullptr))
			{
				int const numConnections = pTriggerNode->getChildCount();
				TriggerConnections connections;
				connections.reserve(numConnections);

				float maxRadius = 0.0f;
				pTriggerNode->getAttr(s_szRadiusAttribute, maxRadius);

				for (int m = 0; m < numConnections; ++m)
				{
					XmlNodeRef const pTriggerImplNode(pTriggerNode->getChild(m));

					if (pTriggerImplNode)
					{
						Impl::ITrigger const* const pITrigger = g_pIImpl->ConstructTrigger(pTriggerImplNode);

						if (pITrigger != nullptr)
						{
							CATLTriggerImpl const* const pTriggerImpl = new CATLTriggerImpl(++m_triggerImplIdCounter, pITrigger);
							connections.push_back(pTriggerImpl);
						}
					}
				}

				connections.shrink_to_fit();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				CTrigger* const pNewTrigger = new CTrigger(triggerId, dataScope, connections, maxRadius, szTriggerName);
#else
				CTrigger* const pNewTrigger = new CTrigger(triggerId, dataScope, connections, maxRadius);
#endif        // INCLUDE_AUDIO_PRODUCTION_CODE

				if (pNewTrigger != nullptr)
				{
					g_triggers[triggerId] = pNewTrigger;
				}
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Trigger "%s" already exists!)", szTriggerName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseDefaultTriggers(XmlNodeRef const pXMLTriggerRoot)
{
	int const numTriggers = pXMLTriggerRoot->getChildCount();

	for (int i = 0; i < numTriggers; ++i)
	{
		XmlNodeRef const pTriggerNode(pXMLTriggerRoot->getChild(i));

		if (pTriggerNode && _stricmp(pTriggerNode->getTag(), s_szTriggerTag) == 0)
		{
			char const* const szTriggerName = pTriggerNode->getAttr(s_szNameAttribute);
			auto const triggerId = static_cast<ControlId const>(StringToId(szTriggerName));
			int const numConnections = pTriggerNode->getChildCount();
			TriggerConnections connections;
			connections.reserve(numConnections);

			for (int j = 0; j < numConnections; ++j)
			{
				XmlNodeRef const pConnectionNode(pTriggerNode->getChild(j));

				if (pConnectionNode != nullptr)
				{
					Impl::ITrigger const* const pITrigger = g_pIImpl->ConstructTrigger(pConnectionNode);

					if (pITrigger != nullptr)
					{
						CATLTriggerImpl const* const pTriggerImpl = new CATLTriggerImpl(++m_triggerImplIdCounter, pITrigger);
						connections.push_back(pTriggerImpl);
					}
				}
			}

			connections.shrink_to_fit();

			switch (triggerId)
			{
			case LoseFocusTriggerId:
				{
					CRY_ASSERT_MESSAGE(g_pLoseFocusTrigger == nullptr, "<Audio> lose focus trigger must be nullptr during initialization.");
					g_pLoseFocusTrigger = new CLoseFocusTrigger(connections);

					break;
				}
			case GetFocusTriggerId:
				{
					CRY_ASSERT_MESSAGE(g_pGetFocusTrigger == nullptr, "<Audio> get focus trigger must be nullptr during initialization.");
					g_pGetFocusTrigger = new CGetFocusTrigger(connections);

					break;
				}
			case MuteAllTriggerId:
				{
					CRY_ASSERT_MESSAGE(g_pMuteAllTrigger == nullptr, "<Audio> mute all trigger must be nullptr during initialization.");
					g_pMuteAllTrigger = new CMuteAllTrigger(connections);

					break;
				}
			case UnmuteAllTriggerId:
				{
					CRY_ASSERT_MESSAGE(g_pUnmuteAllTrigger == nullptr, "<Audio> unmute all trigger must be nullptr during initialization.");
					g_pUnmuteAllTrigger = new CUnmuteAllTrigger(connections);

					break;
				}
			case PauseAllTriggerId:
				{
					CRY_ASSERT_MESSAGE(g_pPauseAllTrigger == nullptr, "<Audio> pause all trigger must be nullptr during initialization.");
					g_pPauseAllTrigger = new CPauseAllTrigger(connections);

					break;
				}
			case ResumeAllTriggerId:
				{
					CRY_ASSERT_MESSAGE(g_pResumeAllTrigger == nullptr, "<Audio> resume all trigger must be nullptr during initialization.");
					g_pResumeAllTrigger = new CResumeAllTrigger(connections);

					break;
				}
			default:
				CRY_ASSERT_MESSAGE(false, R"(The default trigger "%s" does not exist.)", szTriggerName);
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseSwitches(XmlNodeRef const pXMLSwitchRoot, EDataScope const dataScope)
{
	int const numSwitches = pXMLSwitchRoot->getChildCount();

	for (int i = 0; i < numSwitches; ++i)
	{
		XmlNodeRef const pSwitchNode(pXMLSwitchRoot->getChild(i));

		if (pSwitchNode && _stricmp(pSwitchNode->getTag(), s_szSwitchTag) == 0)
		{
			char const* const szSwitchName = pSwitchNode->getAttr(s_szNameAttribute);
			auto const switchId = static_cast<ControlId const>(StringToId(szSwitchName));

			if ((switchId != InvalidControlId) && (stl::find_in_map(g_switches, switchId, nullptr) == nullptr))
			{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				auto const pNewSwitch = new CATLSwitch(switchId, dataScope, szSwitchName);
#else
				auto const pNewSwitch = new CATLSwitch(switchId, dataScope);
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE

				int const numStates = pSwitchNode->getChildCount();

				for (int j = 0; j < numStates; ++j)
				{
					XmlNodeRef const pSwitchStateNode(pSwitchNode->getChild(j));

					if (pSwitchStateNode && _stricmp(pSwitchStateNode->getTag(), s_szStateTag) == 0)
					{
						char const* const szSwitchStateName = pSwitchStateNode->getAttr(s_szNameAttribute);
						auto const switchStateId = static_cast<SwitchStateId const>(StringToId(szSwitchStateName));

						if (switchStateId != InvalidSwitchStateId)
						{
							int const numConnections = pSwitchStateNode->getChildCount();
							CATLSwitchState::ImplPtrVec connections;
							connections.reserve(numConnections);

							for (int k = 0; k < numConnections; ++k)
							{
								XmlNodeRef const pStateImplNode(pSwitchStateNode->getChild(k));

								if (pStateImplNode != nullptr)
								{
									Impl::ISwitchState const* const pISwitchState = g_pIImpl->ConstructSwitchState(pStateImplNode);

									if (pISwitchState != nullptr)
									{
										// Only add the connection if the middleware recognizes the control
										CExternalAudioSwitchStateImpl const* const pExternalSwitchStateImpl = new CExternalAudioSwitchStateImpl(pISwitchState);
										connections.push_back(pExternalSwitchStateImpl);
									}
								}
							}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
							auto const pNewState = new CATLSwitchState(switchId, switchStateId, connections, szSwitchStateName);
#else
							auto const pNewState = new CATLSwitchState(switchId, switchStateId, connections);
#endif          // INCLUDE_AUDIO_PRODUCTION_CODE

							pNewSwitch->audioSwitchStates[switchStateId] = pNewState;
						}
					}
				}

				g_switches[switchId] = pNewSwitch;
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Switch "%s" already exists!)", szSwitchName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseParameters(XmlNodeRef const pXMLParameterRoot, EDataScope const dataScope)
{
	int const numParameters = pXMLParameterRoot->getChildCount();

	for (int i = 0; i < numParameters; ++i)
	{
		XmlNodeRef const pParameterNode(pXMLParameterRoot->getChild(i));

		if (pParameterNode && _stricmp(pParameterNode->getTag(), s_szParameterTag) == 0)
		{
			char const* const szParameterName = pParameterNode->getAttr(s_szNameAttribute);
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
						Impl::IParameter const* const pExternalParameterImpl = g_pIImpl->ConstructParameter(pParameterImplNode);

						if (pExternalParameterImpl != nullptr)
						{
							CParameterImpl const* const pParameterImpl = new CParameterImpl(pExternalParameterImpl);
							connections.push_back(pParameterImpl);
						}
					}
				}

				connections.shrink_to_fit();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				auto const pParameter = new CParameter(parameterId, dataScope, connections, szParameterName);
#else
				auto const pParameter = new CParameter(parameterId, dataScope, connections);
#endif        // INCLUDE_AUDIO_PRODUCTION_CODE

				if (pParameter != nullptr)
				{
					g_parameters[parameterId] = pParameter;
				}
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, R"(Parameter "%s" already exists!)", szParameterName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseDefaultParameters(XmlNodeRef const pXMLParameterRoot)
{
	int const numParameters = pXMLParameterRoot->getChildCount();

	for (int i = 0; i < numParameters; ++i)
	{
		XmlNodeRef const pParameterNode(pXMLParameterRoot->getChild(i));

		if (pParameterNode && _stricmp(pParameterNode->getTag(), s_szParameterTag) == 0)
		{
			char const* const szParameterName = pParameterNode->getAttr(s_szNameAttribute);
			ControlId const parameterId = static_cast<ControlId const>(StringToId(szParameterName));
			int const numConnections = pParameterNode->getChildCount();
			ParameterConnections connections;
			connections.reserve(numConnections);

			for (int j = 0; j < numConnections; ++j)
			{
				XmlNodeRef const pParameterImplNode(pParameterNode->getChild(j));

				if (pParameterImplNode != nullptr)
				{
					Impl::IParameter const* const pExternalParameterImpl = g_pIImpl->ConstructParameter(pParameterImplNode);

					if (pExternalParameterImpl != nullptr)
					{
						CParameterImpl const* const pParameterImpl = new CParameterImpl(pExternalParameterImpl);
						connections.push_back(pParameterImpl);
					}
				}
			}

			connections.shrink_to_fit();

			switch (parameterId)
			{
			case AbsoluteVelocityParameterId:
				{
					CRY_ASSERT_MESSAGE(g_pAbsoluteVelocityParameter == nullptr, "<Audio> absolute velocity parameter must be nullptr during initialization.");
					g_pAbsoluteVelocityParameter = new CAbsoluteVelocityParameter(connections);

					break;
				}
			case RelativeVelocityParameterId:
				{
					CRY_ASSERT_MESSAGE(g_pRelativeVelocityParameter == nullptr, "<Audio> relative velocity parameter must be nullptr during initialization.");
					g_pRelativeVelocityParameter = new CRelativeVelocityParameter(connections);

					break;
				}
			default:
				{
					CRY_ASSERT_MESSAGE(false, R"(The default parameter "%s" does not exist.)", szParameterName);

					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeleteSwitch(CATLSwitch const* const pSwitch)
{
	if (pSwitch != nullptr)
	{
		for (auto const& statePair : pSwitch->audioSwitchStates)
		{
			CATLSwitchState const* const pSwitchState = statePair.second;

			if (pSwitchState != nullptr)
			{
				for (auto const pStateImpl : pSwitchState->m_implPtrs)
				{
					delete pStateImpl;
				}

				delete pSwitchState;
			}
		}

		delete pSwitch;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeletePreloadRequest(CATLPreloadRequest const* const pPreloadRequest)
{
	if (pPreloadRequest != nullptr)
	{
		EDataScope const dataScope = pPreloadRequest->GetDataScope();

		for (auto const fileId : pPreloadRequest->m_fileEntryIds)
		{
			g_fileCacheManager.TryRemoveFileCacheEntry(fileId, dataScope);
		}

		delete pPreloadRequest;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeleteEnvironment(CATLAudioEnvironment const* const pEnvironment)
{
	if (pEnvironment != nullptr)
	{
		for (auto const pEnvImpl : pEnvironment->m_implPtrs)
		{
			delete pEnvImpl;
		}

		delete pEnvironment;
	}
}
} // namespace CryAudio
