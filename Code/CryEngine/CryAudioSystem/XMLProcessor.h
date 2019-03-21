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
	bool           ParseControlsData(char const* const szFolderPath, ContextId const contextId, char const* const szContextName);
	void           ClearControlsData(ContextId const contextId, bool const clearAll);
	void           ParsePreloadsData(char const* const szFolderPath, ContextId const contextId);
	void           ClearPreloadsData(ContextId const contextId, bool const clearAll);
	void           ParseControlsFile(XmlNodeRef const pRootNode, ContextId const contextId);
	void           ParseDefaultControlsFile(XmlNodeRef const pRootNode);

private:

	void ParseTriggers(XmlNodeRef const pXMLTriggerRoot, ContextId const contextId);
	void ParseDefaultTriggers(XmlNodeRef const pXMLTriggerRoot);
	void ParseSwitches(XmlNodeRef const pXMLSwitchRoot, ContextId const contextId);
	void ParseParameters(XmlNodeRef const pXMLParameterRoot, ContextId const contextId);
	void ParsePreloads(XmlNodeRef const pPreloadDataRoot, ContextId const contextId, char const* const szFolderName, uint const version);
	void ParseEnvironments(XmlNodeRef const pEnvironmentRoot, ContextId const contextId);
	void ParseSettings(XmlNodeRef const pRoot, ContextId const contextId);

	void DeletePreloadRequest(CPreloadRequest const* const pPreloadRequest);
};
} // namespace CryAudio
