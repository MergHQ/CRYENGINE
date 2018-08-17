// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryCore/Platform/platform.h>
#include <CryCore/Platform/platform_impl.inl>

#include <IEditor.h>
#include <IPlugin.h>

#include "../EditorCommon/QtViewPane.h"

#include "MainWindow.h"

REGISTER_VIEWPANE_FACTORY(MainWindow, "Interim Behavior Tree Editor", "Tools", false);

class InterimBehaviorTreePlugin : public IPlugin
{
	enum
	{
		Version = 1,
	};

public:

	InterimBehaviorTreePlugin() {}

	int			GetPluginVersion() override                          { return Version; }
	const char* GetPluginName() override                             { return "Interim Behavior Tree Editor"; }
	const char* GetPluginDescription() override						 { return GetPluginName(); }
};

REGISTER_PLUGIN(InterimBehaviorTreePlugin)
