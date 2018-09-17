// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

class XmlNodeRef;

namespace CryAudio
{
class CATLSwitch;
class CATLPreloadRequest;
class CATLAudioEnvironment;

class CAudioXMLProcessor final
{
public:

	CAudioXMLProcessor() = default;

	CAudioXMLProcessor(CAudioXMLProcessor const&) = delete;
	CAudioXMLProcessor(CAudioXMLProcessor&&) = delete;
	CAudioXMLProcessor& operator=(CAudioXMLProcessor const&) = delete;
	CAudioXMLProcessor& operator=(CAudioXMLProcessor&&) = delete;

	void                ParseControlsData(char const* const szFolderPath, EDataScope const dataScope);
	void                ClearControlsData(EDataScope const dataScope);
	void                ParsePreloadsData(char const* const szFolderPath, EDataScope const dataScope);
	void                ClearPreloadsData(EDataScope const dataScope);
	void                ParseControlsFile(XmlNodeRef const pRootNode, EDataScope const dataScope);
	void                ParseDefaultControlsFile(XmlNodeRef const pRootNode);

private:

	void ParseTriggers(XmlNodeRef const pXMLTriggerRoot, EDataScope const dataScope);
	void ParseDefaultTriggers(XmlNodeRef const pXMLTriggerRoot);
	void ParseSwitches(XmlNodeRef const pXMLSwitchRoot, EDataScope const dataScope);
	void ParseParameters(XmlNodeRef const pXMLParameterRoot, EDataScope const dataScope);
	void ParsePreloads(XmlNodeRef const pPreloadDataRoot, EDataScope const dataScope, char const* const szFolderName, uint const version);
	void ParseEnvironments(XmlNodeRef const pAudioEnvironmentRoot, EDataScope const dataScope);
	void ParseSettings(XmlNodeRef const pRoot, EDataScope const dataScope);

	void DeletePreloadRequest(CATLPreloadRequest const* const pPreloadRequest);
	void DeleteEnvironment(CATLAudioEnvironment const* const pEnvironment);
};
} // namespace CryAudio
