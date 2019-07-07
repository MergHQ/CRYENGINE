// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#define CRY_USE_ATL
#define CRY_USE_MFC
#include <CryCore/Platform/CryAtlMfc.h>

#include <IEditorClassFactory.h>
#include "ConsolePlugin.h"
#include "ConsoleWindow.h"
#include "CVarWindow.h"

#define CONSOLE_WINDOW_NAME "Console"
#define CVAR_WINDOW_NAME    "CVars"
#define CATEGORY_NAME       "Console"

#include "QtViewPane.h"

#include <CryCore/Platform/platform_impl.inl>

//single instance
CConsolePlugin* CConsolePlugin::s_pInstance;

REGISTER_VIEWPANE_FACTORY_AND_MENU(CConsoleWindow, "Console", "Tools", false, "Advanced")

REGISTER_PLUGIN(CConsolePlugin);

//create plugin instance
CConsolePlugin::CConsolePlugin() : CEngineListener(GetIEditor() ? GetIEditor()->GetSystem() : NULL)
{
	s_pInstance = this;
	if (GetIEditor())
	{
		Init(CONSOLE_MAX_HISTORY);
	}
}

//release plugin instance
CConsolePlugin::~CConsolePlugin()
{
	s_pInstance = NULL;
}

//deduce type of CVar we're dealing with
inline Messages::EVarType GetVarType(ICVar* pVar)
{
	if (pVar)
	{
		switch (pVar->GetType())
		{
		case ECVarType::Int:
			return Messages::eVarType_Int;
		case ECVarType::Float:
			return Messages::eVarType_Float;
		case ECVarType::String:
			return Messages::eVarType_String;
		case ECVarType::Int64:
			// TODO: full int64 support
			return Messages::eVarType_Int;
			break;
		}
	}
	return Messages::eVarType_None;
}

//handle auto-complete request
Messages::SAutoCompleteReply CConsolePlugin::HandleAutoCompleteRequest(const Messages::SAutoCompleteRequest& req)
{
	RefreshCVarsAndCommands();

	std::vector<Messages::SAutoCompleteReply::SItem> matches;
	AutoComplete(req.query, [&matches](const string& name, ICVar* pVar)
	{
		Messages::SAutoCompleteReply::SItem match;
		match.Set(name, GetVarType(pVar), pVar ? pVar->GetString() : NULL);
		matches.push_back(match);
	});
	Messages::SAutoCompleteReply msg;
	msg.Set(req.query, std::move(matches));
	return msg;
}

//get unique address
string CConsolePlugin::GetUniqueAddress() const
{
	char buf[20];
	static volatile int id;
	int unique = CryInterlockedIncrement(&id);
	cry_sprintf(buf, "Console/%u", unique);
	return buf;
}
