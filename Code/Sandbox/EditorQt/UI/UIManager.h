// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/Scaleform/IFlashUI.h>
#include "LevelIndependentFileMan.h"

class CUIEditor;

class CUIManager : public ILevelIndependentFileModule, public IUIModule
{
public:
	CUIManager();
	~CUIManager();
	void Init();

	bool IsFlashEnabled() const;
	void ReloadActionGraphs(bool bReloadGraphs = false);
	void SaveChangedGraphs();
	bool HasModifications();
	bool NewUIAction(CString& filename);

	void ReloadScripts();

	//ILevelIndependentFileModule
	virtual bool PromptChanges();
	//~ILevelIndependentFileModule

	//IUIModule
	virtual bool EditorAllowReload();
	virtual void EditorReload();
	//~IUIModule

	void       SetEditor(CUIEditor* pEditor) { m_pEditor = pEditor; }
	CUIEditor* GetEditor() const             { return m_pEditor; }

	static float CV_gfx_FlashReloadTime;
	static int   CV_gfx_FlashReloadEnabled;

private:
	CString GetUIActionFolder();

private:
	CUIEditor* m_pEditor;

};
