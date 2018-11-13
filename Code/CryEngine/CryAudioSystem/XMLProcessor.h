// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

class XmlNodeRef;

namespace CryAudio
{
class CSwitch;
class CPreloadRequest;
class CEnvironment;

class CXMLProcessor final
{
public:

	CXMLProcessor() = default;

	CXMLProcessor(CXMLProcessor const&) = delete;
	CXMLProcessor(CXMLProcessor&&) = delete;
	CXMLProcessor& operator=(CXMLProcessor const&) = delete;
	CXMLProcessor& operator=(CXMLProcessor&&) = delete;

	void           ParseSystemData();
	void           ParseControlsData(char const* const szFolderPath, EDataScope const dataScope);
	void           ClearControlsData(EDataScope const dataScope);
	void           ParsePreloadsData(char const* const szFolderPath, EDataScope const dataScope);
	void           ClearPreloadsData(EDataScope const dataScope);
	void           ParseControlsFile(XmlNodeRef const pRootNode, EDataScope const dataScope);
	void           ParseDefaultControlsFile(XmlNodeRef const pRootNode);

private:

	void ParseTriggers(XmlNodeRef const pXMLTriggerRoot, EDataScope const dataScope);
	void ParseDefaultTriggers(XmlNodeRef const pXMLTriggerRoot);
	void ParseSwitches(XmlNodeRef const pXMLSwitchRoot, EDataScope const dataScope);
	void ParseParameters(XmlNodeRef const pXMLParameterRoot, EDataScope const dataScope);
	void ParsePreloads(XmlNodeRef const pPreloadDataRoot, EDataScope const dataScope, char const* const szFolderName, uint const version);
	void ParseEnvironments(XmlNodeRef const pEnvironmentRoot, EDataScope const dataScope);
	void ParseSettings(XmlNodeRef const pRoot, EDataScope const dataScope);

	void DeletePreloadRequest(CPreloadRequest const* const pPreloadRequest);
};
} // namespace CryAudio
