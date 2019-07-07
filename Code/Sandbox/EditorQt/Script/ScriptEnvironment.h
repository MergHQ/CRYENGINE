// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditor.h>
#include <CryScriptSystem/IScriptSystem.h>

class EditorScriptEnvironment
	: public IEditorNotifyListener
	  , public CScriptableBase
{
public:
	EditorScriptEnvironment();
	~EditorScriptEnvironment();
	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
	// ~IEditorNotifyListener

private:
	void RegisterWithScriptSystem();
	void SelectionChanged();

	SmartScriptTable m_selection;
private:
	int Command(IFunctionHandler* pH, const char* commandName);
};
