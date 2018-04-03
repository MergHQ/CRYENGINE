// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "MonoDevelop_Plugin.h"
#include "ToolBox.h"
#include "MonoDevelop_Manager.h"

namespace
{
const char* PLUGIN_GUID = "{C2C05F26-F20E-4480-A296-8E5FCC6B37BF}";
const int PLUGIN_VERSION = 1;
const char* PLUGIN_NAME = "MonoDevelop";
}

CMonoDevelopPlugin::CMonoDevelopPlugin()
{
	CMonoDevelopManager::GetInstance()->Register();
}

void CMonoDevelopPlugin::Release()
{
	CMonoDevelopManager::GetInstance()->Unregister();
	delete this;
}

void        CMonoDevelopPlugin::ShowAbout() {}

const char* CMonoDevelopPlugin::GetPluginGUID()
{
	return PLUGIN_GUID;
}

DWORD CMonoDevelopPlugin::GetPluginVersion()
{
	return PLUGIN_VERSION;
}

const char* CMonoDevelopPlugin::GetPluginName()
{
	return PLUGIN_NAME;
}

bool CMonoDevelopPlugin::CanExitNow()
{
	return true;
}

void CMonoDevelopPlugin::Serialize(FILE* hFile, bool bIsStoring) {}

void CMonoDevelopPlugin::ResetContent()                          {}

bool CMonoDevelopPlugin::CreateUIElements()
{
	return true;
}

void CMonoDevelopPlugin::OnEditorNotify(EEditorNotifyEvent eventId)
{
	if (eventId == eNotify_OnInit)
	{
		//CMonoDevelopManager::Init(); <-- for UI ; not yet necessary
	}
}

