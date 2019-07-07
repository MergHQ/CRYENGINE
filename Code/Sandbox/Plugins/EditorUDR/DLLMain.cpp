// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#define VC_EXTRALEAN
#include <afxwin.h>
#include <afxext.h>

#include <CryCore/Platform/platform.h>
#include <CryCore/Platform/platform_impl.inl>

#include <IEditor.h>
#include "IPlugin.h"
#include "IEditorClassFactory.h"

#include "QtViewPane.h"

#include "Editor/MainEditorWindow.h"


REGISTER_VIEWPANE_FACTORY_AND_MENU(CMainEditorWindow, UDR_EDITOR_NAME, "Game", true, "Advanced")

class CUDRHistoryPlugin : public IPlugin
{
	enum
	{
		Version = 1,
	};

public:
	CUDRHistoryPlugin() = default;

	~CUDRHistoryPlugin() = default;

	virtual int32       GetPluginVersion() override                          { return DWORD(Version); }
	virtual const char* GetPluginName() override                             { return UDR_EDITOR_NAME; }
	virtual const char* GetPluginDescription() override						 { return ""; }
	// ~IPlugin
};

REGISTER_PLUGIN(CUDRHistoryPlugin)

