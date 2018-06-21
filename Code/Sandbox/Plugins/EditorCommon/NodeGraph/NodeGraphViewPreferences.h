// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <EditorFramework/Preferences.h>


struct EDITOR_COMMON_API CNodeGraphViewPreferences : public SPreferencePage
{
	CNodeGraphViewPreferences();
	virtual bool Serialize(yasli::Archive& ar) override;

	Qt::MouseButtons draggingButton;
};

extern CNodeGraphViewPreferences g_NodeGraphViewPreferences;
