// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioXMLProcessor.h"
#include <IAudioImpl.h>
#include <CryString/CryPath.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioXMLProcessor::CAudioXMLProcessor(
  AudioTriggerLookup& triggers,
  AudioRtpcLookup& rtpcs,
  AudioSwitchLookup& switches,
  AudioEnvironmentLookup& environments,
  AudioPreloadRequestLookup& preloadRequests,
  CFileCacheManager& fileCacheMgr)
	: m_triggers(triggers)
	, m_rtpcs(rtpcs)
	, m_switches(switches)
	, m_environments(environments)
	, m_preloadRequests(preloadRequests)
	, m_triggerImplIdCounter(AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED)
	, m_fileCacheMgr(fileCacheMgr)
	, m_pImpl(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::Release()
{
	m_pImpl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseControlsData(char const* const szFolderPath, EAudioDataScope const dataScope)
{
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sRootFolderPath(szFolderPath);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	if (m_pImpl != nullptr)
	{
		sRootFolderPath.TrimRight("/\\");
		CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> sSearch(sRootFolderPath + "/*.xml");
		_finddata_t fd;
		intptr_t handle = gEnv->pCryPak->FindFirst(sSearch.c_str(), &fd);

		if (handle != -1)
		{
			CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> sFileName;

			do
			{
				sFileName = sRootFolderPath.c_str();
				sFileName += "/";
				sFileName += fd.name;

				XmlNodeRef const pATLConfigRoot(GetISystem()->LoadXmlFromFile(sFileName));

				if (pATLConfigRoot)
				{
					if (_stricmp(pATLConfigRoot->getTag(), SATLXMLTags::szRootNodeTag) == 0)
					{
						size_t const nATLConfigChildrenCount = static_cast<size_t>(pATLConfigRoot->getChildCount());

						for (size_t i = 0; i < nATLConfigChildrenCount; ++i)
						{
							XmlNodeRef const pAudioConfigNode(pATLConfigRoot->getChild(i));

							if (pAudioConfigNode)
							{
								char const* const sAudioConfigNodeTag = pAudioConfigNode->getTag();

								if (_stricmp(sAudioConfigNodeTag, SATLXMLTags::szTriggersNodeTag) == 0)
								{
									ParseAudioTriggers(pAudioConfigNode, dataScope);
								}
								else if (_stricmp(sAudioConfigNodeTag, SATLXMLTags::szRtpcsNodeTag) == 0)
								{
									ParseAudioRtpcs(pAudioConfigNode, dataScope);
								}
								else if (_stricmp(sAudioConfigNodeTag, SATLXMLTags::szSwitchesNodeTag) == 0)
								{
									ParseAudioSwitches(pAudioConfigNode, dataScope);
								}
								else if (_stricmp(sAudioConfigNodeTag, SATLXMLTags::szEnvironmentsNodeTag) == 0)
								{
									ParseAudioEnvironments(pAudioConfigNode, dataScope);
								}
								else if (_stricmp(sAudioConfigNodeTag, SATLXMLTags::szPreloadsNodeTag) == 0 ||
								         _stricmp(sAudioConfigNodeTag, SATLXMLTags::szEditorDataTag) == 0)
								{
									// This tag is valid but ignored here.
								}
								else
								{
									g_audioLogger.Log(eAudioLogType_Warning, "Unknown AudioConfig node: %s", sAudioConfigNodeTag);
									CRY_ASSERT(false);
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
	case eAudioDataScope_Global:
		{
			szDataScope = "Global";

			break;
		}
	case eAudioDataScope_LevelSpecific:
		{
			szDataScope = "Level Specific";

			break;
		}
	}

	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	g_audioLogger.Log(eAudioLogType_Warning, "Parsed controls data in \"%s\" for data scope \"%s\" in %.3f ms!", szFolderPath, szDataScope, duration);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParsePreloadsData(char const* const szFolderPath, EAudioDataScope const dataScope)
{
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> rootFolderPath(szFolderPath);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CTimeValue const startTime(gEnv->pTimer->GetAsyncTime());
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	if (m_pImpl != nullptr)
	{
		rootFolderPath.TrimRight("/\\");
		CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> search(rootFolderPath + CRY_NATIVE_PATH_SEPSTR "*.xml");
		_finddata_t fd;
		intptr_t const handle = gEnv->pCryPak->FindFirst(search.c_str(), &fd);

		if (handle != -1)
		{
			CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> fileName;

			do
			{
				fileName = rootFolderPath.c_str();
				fileName += CRY_NATIVE_PATH_SEPSTR;
				fileName += fd.name;

				XmlNodeRef const pATLConfigRoot(GetISystem()->LoadXmlFromFile(fileName));

				if (pATLConfigRoot)
				{
					if (_stricmp(pATLConfigRoot->getTag(), SATLXMLTags::szRootNodeTag) == 0)
					{

						uint versionNumber = 1;
						pATLConfigRoot->getAttr(SATLXMLTags::szATLVersionAttribute, versionNumber);
						size_t const numChildren = static_cast<size_t>(pATLConfigRoot->getChildCount());

						for (size_t i = 0; i < numChildren; ++i)
						{
							XmlNodeRef const pAudioConfigNode(pATLConfigRoot->getChild(i));

							if (pAudioConfigNode)
							{
								char const* const szAudioConfigNodeTag = pAudioConfigNode->getTag();

								if (_stricmp(szAudioConfigNodeTag, SATLXMLTags::szPreloadsNodeTag) == 0)
								{
									size_t const lastSlashIndex = rootFolderPath.rfind(CRY_NATIVE_PATH_SEPSTR[0]);

									if (rootFolderPath.npos != lastSlashIndex)
									{
										CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const folderName(rootFolderPath.substr(lastSlashIndex + 1, rootFolderPath.size()));
										ParseAudioPreloads(pAudioConfigNode, dataScope, folderName.c_str(), versionNumber);
									}
									else
									{
										ParseAudioPreloads(pAudioConfigNode, dataScope, nullptr, versionNumber);
									}
								}
								else if (_stricmp(szAudioConfigNodeTag, SATLXMLTags::szTriggersNodeTag) == 0 ||
								         _stricmp(szAudioConfigNodeTag, SATLXMLTags::szRtpcsNodeTag) == 0 ||
								         _stricmp(szAudioConfigNodeTag, SATLXMLTags::szSwitchesNodeTag) == 0 ||
								         _stricmp(szAudioConfigNodeTag, SATLXMLTags::szEnvironmentsNodeTag) == 0 ||
								         _stricmp(szAudioConfigNodeTag, SATLXMLTags::szEditorDataTag) == 0)
								{
									// These tags are valid but ignored here.
								}
								else
								{
									g_audioLogger.Log(eAudioLogType_Warning, "Unknown AudioConfig node: %s", szAudioConfigNodeTag);
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
	case eAudioDataScope_Global:
		{
			szDataScope = "Global";

			break;
		}
	case eAudioDataScope_LevelSpecific:
		{
			szDataScope = "Level Specific";

			break;
		}
	}

	float const duration = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();
	g_audioLogger.Log(eAudioLogType_Warning, "Parsed preloads data in \"%s\" for data scope \"%s\" in %.3f ms!", szFolderPath, szDataScope, duration);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ClearControlsData(EAudioDataScope const dataScope)
{
	if (m_pImpl != nullptr)
	{
		AudioTriggerLookup::iterator iterTriggers(m_triggers.begin());
		AudioTriggerLookup::const_iterator iterTriggersEnd(m_triggers.end());

		while (iterTriggers != iterTriggersEnd)
		{
			CATLTrigger const* const pTrigger = iterTriggers->second;

			if ((pTrigger->GetDataScope() == dataScope) || dataScope == eAudioDataScope_All)
			{
				DeleteAudioTrigger(pTrigger);
				iterTriggers = m_triggers.erase(iterTriggers);
				iterTriggersEnd = m_triggers.end();
				continue;
			}

			++iterTriggers;
		}

		AudioRtpcLookup::iterator iterParameters(m_rtpcs.begin());
		AudioRtpcLookup::const_iterator iterParametersEnd(m_rtpcs.end());

		while (iterParameters != iterParametersEnd)
		{
			CATLRtpc const* const pParameter = iterParameters->second;

			if ((pParameter->GetDataScope() == dataScope) || dataScope == eAudioDataScope_All)
			{
				DeleteAudioRtpc(pParameter);
				iterParameters = m_rtpcs.erase(iterParameters);
				iterParametersEnd = m_rtpcs.end();
				continue;
			}

			++iterParameters;
		}

		AudioSwitchLookup::iterator iterSwitches(m_switches.begin());
		AudioSwitchLookup::const_iterator iterSwitchesEnd(m_switches.end());

		while (iterSwitches != iterSwitchesEnd)
		{
			CATLSwitch const* const pSwitch = iterSwitches->second;

			if ((pSwitch->GetDataScope() == dataScope) || dataScope == eAudioDataScope_All)
			{
				DeleteAudioSwitch(pSwitch);
				iterSwitches = m_switches.erase(iterSwitches);
				iterSwitchesEnd = m_switches.end();
				continue;
			}

			++iterSwitches;
		}

		AudioEnvironmentLookup::iterator iterEnvironments(m_environments.begin());
		AudioEnvironmentLookup::const_iterator iterEnvironmentsEnd(m_environments.end());

		while (iterEnvironments != iterEnvironmentsEnd)
		{
			CATLAudioEnvironment const* const pEnvironment = iterEnvironments->second;

			if ((pEnvironment->GetDataScope() == dataScope) || dataScope == eAudioDataScope_All)
			{
				DeleteAudioEnvironment(pEnvironment);
				iterEnvironments = m_environments.erase(iterEnvironments);
				iterEnvironmentsEnd = m_environments.end();
				continue;
			}

			++iterEnvironments;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseAudioPreloads(XmlNodeRef const pPreloadDataRoot, EAudioDataScope const dataScope, char const* const szFolderName, uint const version)
{
	LOADING_TIME_PROFILE_SECTION;

	size_t const preloadRequestCount = static_cast<size_t>(pPreloadDataRoot->getChildCount());

	for (size_t i = 0; i < preloadRequestCount; ++i)
	{
		XmlNodeRef const pPreloadRequestNode(pPreloadDataRoot->getChild(i));

		if (pPreloadRequestNode && _stricmp(pPreloadRequestNode->getTag(), SATLXMLTags::szATLPreloadRequestTag) == 0)
		{
			AudioPreloadRequestId audioPreloadRequestID = SATLInternalControlIDs::globalPreloadRequestId;
			char const* szAudioPreloadRequestName = "global_atl_preloads";
			bool const bAutoLoad = (_stricmp(pPreloadRequestNode->getAttr(SATLXMLTags::szATLTypeAttribute), SATLXMLTags::szATLDataLoadType) == 0);

			if (!bAutoLoad)
			{
				szAudioPreloadRequestName = pPreloadRequestNode->getAttr(SATLXMLTags::szATLNameAttribute);
				audioPreloadRequestID = static_cast<AudioPreloadRequestId>(AudioStringToId(szAudioPreloadRequestName));
			}
			else if (dataScope == eAudioDataScope_LevelSpecific)
			{
				szAudioPreloadRequestName = szFolderName;
				audioPreloadRequestID = static_cast<AudioPreloadRequestId>(AudioStringToId(szAudioPreloadRequestName));
			}

			if (audioPreloadRequestID != INVALID_AUDIO_PRELOAD_REQUEST_ID)
			{
				XmlNodeRef pFileListParentNode = nullptr;
				if (version >= 2)
				{
					size_t const platformCount = pPreloadRequestNode->getChildCount();

					for (size_t j = 0; j < platformCount; ++j)
					{
						XmlNodeRef const pPlatformNode(pPreloadRequestNode->getChild(j));

						if (pPlatformNode && _stricmp(pPlatformNode->getAttr(SATLXMLTags::szATLNameAttribute), SATLXMLTags::szPlatform) == 0)
						{
							pFileListParentNode = pPlatformNode;
							break;
						}
					}
				}
				else
				{
					size_t const preloadRequestChidrenCount = static_cast<size_t>(pPreloadRequestNode->getChildCount());

					if (preloadRequestChidrenCount > 1)
					{
						// We need to have at least two children: ATLPlatforms and at least one ATLConfigGroup
						XmlNodeRef const pPlatformsNode(pPreloadRequestNode->getChild(0));
						char const* szATLConfigGroupName = nullptr;

						if (pPlatformsNode && _stricmp(pPlatformsNode->getTag(), SATLXMLTags::szATLPlatformsTag) == 0)
						{
							size_t const platformCount = pPlatformsNode->getChildCount();

							for (size_t j = 0; j < platformCount; ++j)
							{
								XmlNodeRef const pPlatformNode(pPlatformsNode->getChild(j));

								if (pPlatformNode && _stricmp(pPlatformNode->getAttr(SATLXMLTags::szATLNameAttribute), SATLXMLTags::szPlatform) == 0)
								{
									szATLConfigGroupName = pPlatformNode->getAttr(SATLXMLTags::szATLConfigGroupAttribute);
									break;
								}
							}
						}

						if (szATLConfigGroupName != nullptr)
						{
							for (size_t j = 1; j < preloadRequestChidrenCount; ++j)
							{
								XmlNodeRef const pConfigGroupNode(pPreloadRequestNode->getChild(j));
								if (_stricmp(pConfigGroupNode->getAttr(SATLXMLTags::szATLNameAttribute), szATLConfigGroupName) == 0)
								{
									pFileListParentNode = pConfigGroupNode;
									break;
								}
							}
						}
					}
				}

				if (pFileListParentNode)
				{
					// Found the config group corresponding to the specified platform.
					size_t const fileCount = static_cast<size_t>(pFileListParentNode->getChildCount());

					CATLPreloadRequest::FileEntryIds cFileEntryIDs;
					cFileEntryIDs.reserve(fileCount);

					for (size_t k = 0; k < fileCount; ++k)
					{
						AudioFileEntryId const id = m_fileCacheMgr.TryAddFileCacheEntry(pFileListParentNode->getChild(k), dataScope, bAutoLoad);

						if (id != INVALID_AUDIO_FILE_ENTRY_ID)
						{
							cFileEntryIDs.push_back(id);
						}
						else
						{
							g_audioLogger.Log(eAudioLogType_Warning, "Preload request \"%s\" could not create file entry from tag \"%s\"!", szAudioPreloadRequestName, pFileListParentNode->getChild(k)->getTag());
						}
					}

					CATLPreloadRequest* pPreloadRequest = stl::find_in_map(m_preloadRequests, audioPreloadRequestID, nullptr);

					if (pPreloadRequest == nullptr)
					{
						POOL_NEW(CATLPreloadRequest, pPreloadRequest)(audioPreloadRequestID, dataScope, bAutoLoad, cFileEntryIDs);

						if (pPreloadRequest != nullptr)
						{
							m_preloadRequests[audioPreloadRequestID] = pPreloadRequest;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
							pPreloadRequest->m_name = szAudioPreloadRequestName;
#endif        // INCLUDE_AUDIO_PRODUCTION_CODE
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
				g_audioLogger.Log(eAudioLogType_Error, "Preload request \"%s\" already exists! Skipping this entry!", szAudioPreloadRequestName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ClearPreloadsData(EAudioDataScope const dataScope)
{
	if (m_pImpl != nullptr)
	{
		AudioPreloadRequestLookup::iterator iRemover = m_preloadRequests.begin();
		AudioPreloadRequestLookup::const_iterator const iEnd = m_preloadRequests.end();

		while (iRemover != iEnd)
		{
			CATLPreloadRequest const* const pRequest = iRemover->second;

			if ((pRequest->GetDataScope() == dataScope) || dataScope == eAudioDataScope_All)
			{
				DeleteAudioPreloadRequest(pRequest);
				m_preloadRequests.erase(iRemover++);
			}
			else
			{
				++iRemover;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseAudioEnvironments(XmlNodeRef const pAudioEnvironmentRoot, EAudioDataScope const dataScope)
{
	size_t const numAudioEnvironments = static_cast<size_t>(pAudioEnvironmentRoot->getChildCount());

	for (size_t i = 0; i < numAudioEnvironments; ++i)
	{
		XmlNodeRef const pAudioEnvironmentNode(pAudioEnvironmentRoot->getChild(i));

		if (pAudioEnvironmentNode && _stricmp(pAudioEnvironmentNode->getTag(), SATLXMLTags::szATLEnvironmentTag) == 0)
		{
			char const* const szAudioEnvironmentName = pAudioEnvironmentNode->getAttr(SATLXMLTags::szATLNameAttribute);
			AudioEnvironmentId const audioEnvironmentId = static_cast<AudioEnvironmentId const>(AudioStringToId(szAudioEnvironmentName));

			if ((audioEnvironmentId != INVALID_AUDIO_CONTROL_ID) && (stl::find_in_map(m_environments, audioEnvironmentId, nullptr) == nullptr))
			{
				//there is no entry yet with this ID in the container
				size_t const numAudioEnvironmentChildren = static_cast<size_t>(pAudioEnvironmentNode->getChildCount());

				CATLAudioEnvironment::ImplPtrVec implPtrs;
				implPtrs.reserve(numAudioEnvironmentChildren);

				for (size_t j = 0; j < numAudioEnvironmentChildren; ++j)
				{
					XmlNodeRef const pEnvironmentImplNode(pAudioEnvironmentNode->getChild(j));

					if (pEnvironmentImplNode)
					{
						IAudioEnvironment const* pEnvirnomentImplData = nullptr;
						EAudioSubsystem receiver = eAudioSubsystem_None;

						if (_stricmp(pEnvironmentImplNode->getTag(), SATLXMLTags::szATLEnvironmentRequestTag) == 0)
						{
							pEnvirnomentImplData = NewInternalAudioEnvironment(pEnvironmentImplNode);
							receiver = eAudioSubsystem_AudioInternal;
						}
						else
						{
							pEnvirnomentImplData = m_pImpl->NewAudioEnvironment(pEnvironmentImplNode);
							receiver = eAudioSubsystem_AudioImpl;
						}

						if (pEnvirnomentImplData != nullptr)
						{
							POOL_NEW_CREATE(CATLEnvironmentImpl, pEnvirnomentImpl)(receiver, pEnvirnomentImplData);
							implPtrs.push_back(pEnvirnomentImpl);
						}
						else
						{
							g_audioLogger.Log(eAudioLogType_Warning, "Could not parse an Environment Implementation with XML tag %s", pEnvironmentImplNode->getTag());
						}
					}
				}

				implPtrs.shrink_to_fit();

				if (!implPtrs.empty())
				{
					POOL_NEW_CREATE(CATLAudioEnvironment, pNewEnvironment)(audioEnvironmentId, dataScope, implPtrs);

					if (pNewEnvironment != nullptr)
					{
						m_environments[audioEnvironmentId] = pNewEnvironment;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
						pNewEnvironment->m_name = szAudioEnvironmentName;
#endif      // INCLUDE_AUDIO_PRODUCTION_CODE
					}
				}
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Error, "AudioEnvironment \"%s\" already exists!", szAudioEnvironmentName);
				CRY_ASSERT(false);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseAudioTriggers(XmlNodeRef const pXMLTriggerRoot, EAudioDataScope const dataScope)
{
	size_t const numAudioTriggersChildren = static_cast<size_t>(pXMLTriggerRoot->getChildCount());

	for (size_t i = 0; i < numAudioTriggersChildren; ++i)
	{
		XmlNodeRef const pAudioTriggerNode(pXMLTriggerRoot->getChild(i));

		if (pAudioTriggerNode && _stricmp(pAudioTriggerNode->getTag(), SATLXMLTags::szATLTriggerTag) == 0)
		{
			char const* const szAudioTriggerName = pAudioTriggerNode->getAttr(SATLXMLTags::szATLNameAttribute);
			AudioControlId const audioTriggerId = static_cast<AudioControlId const>(AudioStringToId(szAudioTriggerName));

			if ((audioTriggerId != INVALID_AUDIO_CONTROL_ID) && (stl::find_in_map(m_triggers, audioTriggerId, nullptr) == nullptr))
			{
				size_t const numAudioTriggerChildren = static_cast<size_t>(pAudioTriggerNode->getChildCount());
				CATLTrigger::ImplPtrVec implPtrs;
				implPtrs.reserve(numAudioTriggerChildren);

				float maxRadius = 0.0f;
				pAudioTriggerNode->getAttr(SATLXMLTags::szATLRadiusAttribute, maxRadius);

				float occlusionFadeOutDistance = 0.0f;
				pAudioTriggerNode->getAttr(SATLXMLTags::szATLOcclusionFadeOutDistanceAttribute, occlusionFadeOutDistance);

				for (size_t m = 0; m < numAudioTriggerChildren; ++m)
				{
					XmlNodeRef const pTriggerImplNode(pAudioTriggerNode->getChild(m));

					if (pTriggerImplNode)
					{
						IAudioTrigger const* pAudioTrigger = nullptr;
						EAudioSubsystem receiver = eAudioSubsystem_None;

						if (_stricmp(pTriggerImplNode->getTag(), SATLXMLTags::szATLTriggerRequestTag) == 0)
						{
							pAudioTrigger = NewInternalAudioTrigger(pTriggerImplNode);

							receiver = eAudioSubsystem_AudioInternal;
						}
						else
						{
							pAudioTrigger = m_pImpl->NewAudioTrigger(pTriggerImplNode);
							receiver = eAudioSubsystem_AudioImpl;
						}

						if (pAudioTrigger != nullptr)
						{
							POOL_NEW_CREATE(CATLTriggerImpl, pTriggerImpl)(++m_triggerImplIdCounter, audioTriggerId, receiver, pAudioTrigger);
							implPtrs.push_back(pTriggerImpl);
						}
						else
						{
							g_audioLogger.Log(eAudioLogType_Warning, "Could not parse a Trigger Implementation with XML tag %s", pTriggerImplNode->getTag());
						}
					}
				}

				implPtrs.shrink_to_fit();

				POOL_NEW_CREATE(CATLTrigger, pNewTrigger)(audioTriggerId, dataScope, implPtrs, maxRadius, occlusionFadeOutDistance);

				if (pNewTrigger != nullptr)
				{
					m_triggers[audioTriggerId] = pNewTrigger;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
					pNewTrigger->m_name = szAudioTriggerName;
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE
				}
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Error, "trigger \"%s\" already exists!", szAudioTriggerName);
				CRY_ASSERT(false);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseAudioSwitches(XmlNodeRef const pXMLSwitchRoot, EAudioDataScope const dataScope)
{
	size_t const numAudioSwitchChildren = static_cast<size_t>(pXMLSwitchRoot->getChildCount());

	for (size_t i = 0; i < numAudioSwitchChildren; ++i)
	{
		XmlNodeRef const pATLSwitchNode(pXMLSwitchRoot->getChild(i));

		if (pATLSwitchNode && _stricmp(pATLSwitchNode->getTag(), SATLXMLTags::szATLSwitchTag) == 0)
		{
			char const* const szAudioSwitchName = pATLSwitchNode->getAttr(SATLXMLTags::szATLNameAttribute);
			AudioControlId const audioSwitchId = static_cast<AudioControlId const>(AudioStringToId(szAudioSwitchName));

			if ((audioSwitchId != INVALID_AUDIO_CONTROL_ID) && (stl::find_in_map(m_switches, audioSwitchId, nullptr) == nullptr))
			{
				POOL_NEW_CREATE(CATLSwitch, pNewSwitch)(audioSwitchId, dataScope);
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pNewSwitch->m_name = szAudioSwitchName;
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

				size_t const numAudioSwitchStates = static_cast<size_t>(pATLSwitchNode->getChildCount());

				for (size_t j = 0; j < numAudioSwitchStates; ++j)
				{
					XmlNodeRef const pATLSwitchStateNode(pATLSwitchNode->getChild(j));

					if (pATLSwitchStateNode && _stricmp(pATLSwitchStateNode->getTag(), SATLXMLTags::szATLSwitchStateTag) == 0)
					{
						char const* const szAudioSwitchStateName = pATLSwitchStateNode->getAttr(SATLXMLTags::szATLNameAttribute);
						AudioSwitchStateId const audioSwitchStateId = static_cast<AudioSwitchStateId const>(AudioStringToId(szAudioSwitchStateName));

						if (audioSwitchStateId != INVALID_AUDIO_SWITCH_STATE_ID)
						{
							size_t const numAudioSwitchStateImpl = static_cast<size_t>(pATLSwitchStateNode->getChildCount());

							CATLSwitchState::ImplPtrVec switchStateImplVec;
							switchStateImplVec.reserve(numAudioSwitchStateImpl);

							for (size_t k = 0; k < numAudioSwitchStateImpl; ++k)
							{
								XmlNodeRef const pStateImplNode(pATLSwitchStateNode->getChild(k));

								if (pStateImplNode)
								{
									char const* const szStateImplNodeTag = pStateImplNode->getTag();
									IAudioSwitchState const* pNewSwitchStateImplData = nullptr;
									EAudioSubsystem audioSubsystem = eAudioSubsystem_None;

									if (_stricmp(szStateImplNodeTag, SATLXMLTags::szATLSwitchRequestTag) == 0)
									{
										pNewSwitchStateImplData = NewInternalAudioSwitchState(pStateImplNode);
										audioSubsystem = eAudioSubsystem_AudioInternal;
									}
									else
									{
										pNewSwitchStateImplData = m_pImpl->NewAudioSwitchState(pStateImplNode);
										audioSubsystem = eAudioSubsystem_AudioImpl;
									}

									if (pNewSwitchStateImplData != nullptr)
									{
										POOL_NEW_CREATE(CATLSwitchStateImpl, pSwitchStateImpl)(audioSubsystem, pNewSwitchStateImplData);
										switchStateImplVec.push_back(pSwitchStateImpl);
									}
								}
							}

							POOL_NEW_CREATE(CATLSwitchState, pNewState)(audioSwitchId, audioSwitchStateId, switchStateImplVec);
							pNewSwitch->audioSwitchStates[audioSwitchStateId] = pNewState;
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
							pNewState->m_name = szAudioSwitchStateName;
#endif        // INCLUDE_AUDIO_PRODUCTION_CODE
						}
					}
				}

				m_switches[audioSwitchId] = pNewSwitch;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::ParseAudioRtpcs(XmlNodeRef const pXMLRtpcRoot, EAudioDataScope const dataScope)
{
	size_t const numAudioParameterChildren = static_cast<size_t>(pXMLRtpcRoot->getChildCount());

	for (size_t i = 0; i < numAudioParameterChildren; ++i)
	{
		XmlNodeRef const pAudioParameterNode(pXMLRtpcRoot->getChild(i));

		if (pAudioParameterNode && _stricmp(pAudioParameterNode->getTag(), SATLXMLTags::szATLRtpcTag) == 0)
		{
			char const* const szAudioParameterName = pAudioParameterNode->getAttr(SATLXMLTags::szATLNameAttribute);
			AudioControlId const audioParameterId = static_cast<AudioControlId const>(AudioStringToId(szAudioParameterName));

			if ((audioParameterId != INVALID_AUDIO_CONTROL_ID) && (stl::find_in_map(m_rtpcs, audioParameterId, nullptr) == nullptr))
			{
				size_t const numParameterNodeChildren = static_cast<size_t>(pAudioParameterNode->getChildCount());
				CATLRtpc::ImplPtrVec implPtrs;
				implPtrs.reserve(numParameterNodeChildren);

				for (size_t j = 0; j < numParameterNodeChildren; ++j)
				{
					XmlNodeRef const pParameterImplNode(pAudioParameterNode->getChild(j));

					if (pParameterImplNode)
					{
						char const* const szParameterImplNodeTag = pParameterImplNode->getTag();
						IAudioRtpc const* pNewParameterImplData = nullptr;
						EAudioSubsystem receiver = eAudioSubsystem_None;

						if (_stricmp(szParameterImplNodeTag, SATLXMLTags::szATLRtpcRequestTag) == 0)
						{
							pNewParameterImplData = NewInternalAudioRtpc(pParameterImplNode);
							receiver = eAudioSubsystem_AudioInternal;
						}
						else
						{
							pNewParameterImplData = m_pImpl->NewAudioRtpc(pParameterImplNode);
							receiver = eAudioSubsystem_AudioImpl;
						}

						if (pNewParameterImplData != nullptr)
						{
							POOL_NEW_CREATE(CATLRtpcImpl, pParameterImpl)(receiver, pNewParameterImplData);
							implPtrs.push_back(pParameterImpl);
						}
					}
				}

				implPtrs.shrink_to_fit();

				POOL_NEW_CREATE(CATLRtpc, pParameter)(audioParameterId, dataScope, implPtrs);

				if (pParameter != nullptr)
				{
					m_rtpcs[audioParameterId] = pParameter;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
					pParameter->m_name = szAudioParameterName;
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
IAudioTrigger const* CAudioXMLProcessor::NewInternalAudioTrigger(XmlNodeRef const pXMLTriggerRoot)
{
	//TODO: implement
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
IAudioRtpc const* CAudioXMLProcessor::NewInternalAudioRtpc(XmlNodeRef const pXMLRtpcRoot)
{
	//TODO: implement
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
IAudioSwitchState const* CAudioXMLProcessor::NewInternalAudioSwitchState(XmlNodeRef const pXMLSwitchRoot)
{
	SATLSwitchStateImplData_internal* pSwitchStateImpl = nullptr;

	char const* const sInternalSwitchNodeName = pXMLSwitchRoot->getAttr(SATLXMLTags::szATLNameAttribute);

	if ((sInternalSwitchNodeName != nullptr) && (sInternalSwitchNodeName[0] != 0) && (pXMLSwitchRoot->getChildCount() == 1))
	{
		XmlNodeRef const pValueNode(pXMLSwitchRoot->getChild(0));

		if (pValueNode && _stricmp(pValueNode->getTag(), SATLXMLTags::szATLValueTag) == 0)
		{
			char const* sInternalSwitchStateName = pValueNode->getAttr(SATLXMLTags::szATLNameAttribute);

			if ((sInternalSwitchStateName != nullptr) && (sInternalSwitchStateName[0] != 0))
			{
				AudioControlId const nATLInternalSwitchID = static_cast<AudioControlId>(AudioStringToId(sInternalSwitchNodeName));
				AudioSwitchStateId const nATLInternalStateID = static_cast<AudioSwitchStateId>(AudioStringToId(sInternalSwitchStateName));
				POOL_NEW(SATLSwitchStateImplData_internal, pSwitchStateImpl)(nATLInternalSwitchID, nATLInternalStateID);
			}
		}
	}
	else
	{
		g_audioLogger.Log(
		  eAudioLogType_Warning,
		  "An ATLSwitchRequest %s inside ATLSwitchState needs to have exactly one ATLValue.",
		  sInternalSwitchNodeName);
	}

	return pSwitchStateImpl;
}

//////////////////////////////////////////////////////////////////////////
IAudioEnvironment const* CAudioXMLProcessor::NewInternalAudioEnvironment(XmlNodeRef const pXMLEnvironmentRoot)
{
	// TODO: implement
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeleteAudioTrigger(CATLTrigger const* const pOldTrigger)
{
	if (pOldTrigger != nullptr)
	{
		for (auto const pTriggerImpl : pOldTrigger->m_implPtrs)
		{
			if (pTriggerImpl->GetReceiver() == eAudioSubsystem_AudioInternal)
			{
				POOL_FREE_CONST(pTriggerImpl->m_pImplData);
			}
			else
			{
				m_pImpl->DeleteAudioTrigger(pTriggerImpl->m_pImplData);
			}

			POOL_FREE_CONST(pTriggerImpl);
		}

		POOL_FREE_CONST(pOldTrigger);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeleteAudioRtpc(CATLRtpc const* const pOldRtpc)
{
	if (pOldRtpc != nullptr)
	{
		for (auto const pRtpcImpl : pOldRtpc->m_implPtrs)
		{
			if (pRtpcImpl->GetReceiver() == eAudioSubsystem_AudioInternal)
			{
				POOL_FREE_CONST(pRtpcImpl->m_pImplData);
			}
			else
			{
				m_pImpl->DeleteAudioRtpc(pRtpcImpl->m_pImplData);
			}

			POOL_FREE_CONST(pRtpcImpl);
		}

		POOL_FREE_CONST(pOldRtpc);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeleteAudioSwitch(CATLSwitch const* const pOldSwitch)
{
	if (pOldSwitch != nullptr)
	{
		for (auto const& statePair : pOldSwitch->audioSwitchStates)
		{
			CATLSwitchState const* const pSwitchState = statePair.second;

			if (pSwitchState != nullptr)
			{
				for (auto const pStateImpl : pSwitchState->m_implPtrs)
				{
					if (pStateImpl->GetReceiver() == eAudioSubsystem_AudioInternal)
					{
						POOL_FREE_CONST(pStateImpl->m_pImplData);
					}
					else
					{
						m_pImpl->DeleteAudioSwitchState(pStateImpl->m_pImplData);
					}

					POOL_FREE_CONST(pStateImpl);
				}

				POOL_FREE_CONST(pSwitchState);
			}
		}

		POOL_FREE_CONST(pOldSwitch);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeleteAudioPreloadRequest(CATLPreloadRequest const* const pOldPreloadRequest)
{
	if (pOldPreloadRequest != nullptr)
	{
		EAudioDataScope const dataScope = pOldPreloadRequest->GetDataScope();

		for (auto const fileId : pOldPreloadRequest->m_fileEntryIds)
		{
			m_fileCacheMgr.TryRemoveFileCacheEntry(fileId, dataScope);
		}

		POOL_FREE_CONST(pOldPreloadRequest);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioXMLProcessor::DeleteAudioEnvironment(CATLAudioEnvironment const* const pOldEnvironment)
{
	if (pOldEnvironment != nullptr)
	{
		for (auto const pEnvImpl : pOldEnvironment->m_implPtrs)
		{
			if (pEnvImpl->GetReceiver() == eAudioSubsystem_AudioInternal)
			{
				POOL_FREE_CONST(pEnvImpl->m_pImplData);
			}
			else
			{
				m_pImpl->DeleteAudioEnvironment(pEnvImpl->m_pImplData);
			}

			POOL_FREE_CONST(pEnvImpl);
		}

		POOL_FREE_CONST(pOldEnvironment);
	}
}
