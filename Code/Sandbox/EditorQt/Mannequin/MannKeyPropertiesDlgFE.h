// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Dialogs/ToolbarDialog.h"
#include "SequencerKeyPropertiesDlg.h"
#include "Dialogs/BaseFrameWnd.h"

struct IAnimationDatabase;
struct SMannequinContexts;

class SequencerKeys;
class CMannKeyPropertiesDlgFE : public CSequencerKeyPropertiesDlg
{
public:
	CMannKeyPropertiesDlgFE();

	enum { IDD = IDD_PANEL_PROPERTIES_FE };

	//////////////////////////////////////////////////////////////////////////
	// ISequencerEventsListener interface implementation
	//////////////////////////////////////////////////////////////////////////
	virtual void OnKeySelectionChange();
	//////////////////////////////////////////////////////////////////////////

	void ForceUpdate()
	{
		m_forceUpdate = true;
	}

	void OnUpdate();

	void RedrawProps();

	void RepopulateUI(CSequencerKeyUIControls* controls);

	SMannequinContexts* m_contexts;

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

	int  m_lastPropertyType;
	bool m_forceUpdate;
};
