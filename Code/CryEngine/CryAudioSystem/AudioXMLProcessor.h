// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include "FileCacheManager.h"

class CAudioXMLProcessor final
{
public:

	explicit CAudioXMLProcessor(
	  AudioTriggerLookup& triggers,
	  AudioRtpcLookup& rtpcs,
	  AudioSwitchLookup& switches,
	  AudioEnvironmentLookup& environments,
	  AudioPreloadRequestLookup& preloadRequests,
	  CFileCacheManager& fileCacheMgr);

	CAudioXMLProcessor(CAudioXMLProcessor const&) = delete;
	CAudioXMLProcessor(CAudioXMLProcessor&&) = delete;
	CAudioXMLProcessor& operator=(CAudioXMLProcessor const&) = delete;
	CAudioXMLProcessor& operator=(CAudioXMLProcessor&&) = delete;

	void Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void Release();

	void ParseControlsData(char const* const szFolderPath, EAudioDataScope const dataScope);
	void ClearControlsData(EAudioDataScope const dataScope);
	void ParsePreloadsData(char const* const szFolderPath, EAudioDataScope const dataScope);
	void ClearPreloadsData(EAudioDataScope const dataScope);

private:

	void                                     ParseAudioTriggers(XmlNodeRef const pXMLTriggerRoot, EAudioDataScope const dataScope);
	void                                     ParseAudioSwitches(XmlNodeRef const pXMLSwitchRoot, EAudioDataScope const dataScope);
	void                                     ParseAudioRtpcs(XmlNodeRef const pXMLRtpcRoot, EAudioDataScope const dataScope);
	void                                     ParseAudioPreloads(XmlNodeRef const pPreloadDataRoot, EAudioDataScope const dataScope, char const* const szFolderName, uint const version);
	void                                     ParseAudioEnvironments(XmlNodeRef const pAudioEnvironmentRoot, EAudioDataScope const dataScope);

	CryAudio::Impl::IAudioTrigger const*     NewInternalAudioTrigger(XmlNodeRef const pXMLTriggerRoot);
	CryAudio::Impl::IAudioRtpc const*        NewInternalAudioRtpc(XmlNodeRef const pXMLRtpcRoot);
	CryAudio::Impl::IAudioSwitchState const* NewInternalAudioSwitchState(XmlNodeRef const pXMLSwitchRoot);
	CryAudio::Impl::IAudioEnvironment const* NewInternalAudioEnvironment(XmlNodeRef const pXMLEnvironmentRoot);

	void                                     DeleteAudioTrigger(CATLTrigger const* const pOldTrigger);
	void                                     DeleteAudioRtpc(CATLRtpc const* const pOldRtpc);
	void                                     DeleteAudioSwitch(CATLSwitch const* const pOldSwitch);
	void                                     DeleteAudioPreloadRequest(CATLPreloadRequest const* const pOldPreloadRequest);
	void                                     DeleteAudioEnvironment(CATLAudioEnvironment const* const pOldEnvironment);

	AudioTriggerLookup&         m_triggers;
	AudioRtpcLookup&            m_rtpcs;
	AudioSwitchLookup&          m_switches;
	AudioEnvironmentLookup&     m_environments;
	AudioPreloadRequestLookup&  m_preloadRequests;
	AudioTriggerImplId          m_triggerImplIdCounter;
	CFileCacheManager&          m_fileCacheMgr;
	CryAudio::Impl::IAudioImpl* m_pImpl;
};
