// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Dialogs/ToolbarDialog.h"

class CMannequinModelViewport;
class CMannDopeSheet;
class CMannNodesCtrl;

class CMannequinEditorPage : public CToolbarDialog
{
	DECLARE_DYNAMIC(CMannequinEditorPage)

public:
	CMannequinEditorPage(UINT nIDTemplate, CWnd* pParentWnd = NULL);

	virtual CMannequinModelViewport* ModelViewport() const { return NULL; }
	virtual CMannDopeSheet*          TrackPanel()          { return NULL; }
	virtual CMannNodesCtrl*          Nodes()               { return NULL; }

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void ValidateToolbarButtonsState() {}
};
