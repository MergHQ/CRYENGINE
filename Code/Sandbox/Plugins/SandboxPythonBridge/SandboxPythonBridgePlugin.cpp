// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SandboxPythonBridgePlugin.h"

namespace PluginInfo
{
	const char* kName = "Sandbox Python Bridge";
	const char* kGUID = "{33C4B150-679F-49A2-81C1-0C594AE37A86}";
	const int kVersion = 0;
}

SandboxPythonBridgePlugin::SandboxPythonBridgePlugin()
{
	RegisterPlugin();
}


void SandboxPythonBridgePlugin::Release()
{
	UnregisterPlugin();
	delete this;
}

void SandboxPythonBridgePlugin::ShowAbout()
{

}

const char* SandboxPythonBridgePlugin::GetPluginGUID()
{
	return PluginInfo::kGUID;
}

DWORD SandboxPythonBridgePlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* SandboxPythonBridgePlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool SandboxPythonBridgePlugin::CanExitNow()
{
	return true;
}

void SandboxPythonBridgePlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{

}
