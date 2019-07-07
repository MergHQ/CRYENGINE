// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>
#include "NodeGraphViewPreferences.h"

#include <CrySerialization/yasli/decorators/Range.h>
#include <CrySerialization/StringList.h>

CNodeGraphViewPreferences g_NodeGraphViewPreferences;
REGISTER_PREFERENCES_PAGE_PTR(CNodeGraphViewPreferences, &g_NodeGraphViewPreferences)

CNodeGraphViewPreferences::CNodeGraphViewPreferences()
	: SPreferencePage("General", "Node Graph/General")
	, draggingButton(Qt::RightButton)
{
}

bool CNodeGraphViewPreferences::Serialize(yasli::Archive& ar)
{
	char currResString[16];
	sprintf(currResString, "%s", draggingButton == Qt::RightButton ? "Right button" : "Middle button");

	Serialization::StringList draggingButtonOptions;

	draggingButtonOptions.push_back("Right button");
	draggingButtonOptions.push_back("Middle button");
	
	Serialization::StringListValue draggingButtonStr(draggingButtonOptions, std::max(draggingButtonOptions.find(currResString), 0));

	ar.openBlock("general", "General");
	ar(draggingButtonStr, "draggingButton", "Dragging button");
	ar.closeBlock();

	if (ar.isInput())
	{
		draggingButton = (string(draggingButtonStr.c_str()) == "Right button") ? Qt::RightButton : Qt::MiddleButton;
	}

	return true;
}