// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "MFCToolsPlugin.h"

#include "IPlugin.h"
#include <CryCore/Platform/platform_impl.inl>

static IEditor* g_editor = nullptr;
IEditor* GetIEditor() { return g_editor; }

void MFCToolsPlugin::SetEditor(IEditor* editor)
{
	g_editor = editor;
	auto system = GetIEditor()->GetSystem();
	gEnv = system->GetGlobalEnvironment();
}

void MFCToolsPlugin::Initialize()
{
	ModuleInitISystem(GetIEditor()->GetSystem(), "MFCToolsPlugin");
	RegisterPlugin();
}

void MFCToolsPlugin::Deinitialize()
{
	UnregisterPlugin();
	g_editor = nullptr;
	gEnv = nullptr;
}
