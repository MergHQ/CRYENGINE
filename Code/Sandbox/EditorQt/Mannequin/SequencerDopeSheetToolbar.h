// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CSequencerDopeSheetToolbar;

#include "Controls/DlgBars.h"
#include "SequencerDopeSheetBase.h"

class CSequencerDopeSheetToolbar : public CDlgToolBar
{

public:
	CSequencerDopeSheetToolbar();
	virtual ~CSequencerDopeSheetToolbar();
	virtual void InitToolbar();
	virtual void SetTime(float fTime, float fFps);

protected:
	CStatic m_timeWindow;
	float   m_lastTime;

};
