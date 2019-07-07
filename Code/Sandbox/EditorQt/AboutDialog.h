// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "Controls/EditorDialog.h"
#include "Version.h"

#include <QLabel>

class CAboutDialog : public CEditorDialog
{
public:
	CAboutDialog();
	~CAboutDialog();

	void SetVersion(const Version& version);

protected:
	QLabel* m_versionText;
	QLabel* m_miscInfoLabel;

private:
	Version m_version;
};
