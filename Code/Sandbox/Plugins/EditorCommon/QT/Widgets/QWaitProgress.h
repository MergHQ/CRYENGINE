// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//  QWaitProgress class adds information about lengthy process
//		Usage:
//		QWaitProgress wait;
//		wait.SetText("Long");
//		wait.SetProgress(35); // 35 percent.

#include "EditorCommonAPI.h"

class QProgressBar;

// DEPRECATED DO NOT USE!
// Will be replaced by the notification and task ui system
class EDITOR_COMMON_API CWaitProgress
{
public:
	explicit CWaitProgress(const char* szText, bool bStart = true);
	~CWaitProgress();

	void          Start();
	void          Stop();
	//! @return true to continue, false to abort lengthy operation.
	bool          Step(int nPercentage = -1, bool bProcessEvents = false);

	void          SetText(const char* szText);
	const string& GetProcessName() { return m_strText;  }

protected:

	string      m_strText;
	string      m_processName;
	bool        m_bStarted;
	bool        m_bIgnore;
	int         m_percent;
	static bool s_bInProgressNow;
};
