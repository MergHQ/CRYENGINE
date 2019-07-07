// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CEditorAutoLevelLoadTest : public IEditorNotifyListener
{
public:
	static CEditorAutoLevelLoadTest& Instance();
private:
	CEditorAutoLevelLoadTest();
	virtual ~CEditorAutoLevelLoadTest();

	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
};
