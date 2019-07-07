// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct ILevelIndependentFileModule
{
	virtual ~ILevelIndependentFileModule() {}
	// this function should prompt some message box if changed files need to be saved
	// if return false, editor will not continue with current action (e.g close the editor)
	virtual bool PromptChanges() = 0;
};

class CLevelIndependentFileMan
{
public:
	CLevelIndependentFileMan();
	~CLevelIndependentFileMan();

	bool PromptChangedFiles();

	void RegisterModule(ILevelIndependentFileModule* pModule);
	void UnregisterModule(ILevelIndependentFileModule* pModule);
private:
	std::vector<ILevelIndependentFileModule*> m_Modules;
};
