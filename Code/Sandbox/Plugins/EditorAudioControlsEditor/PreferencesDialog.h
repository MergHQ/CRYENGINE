// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controls/EditorDialog.h"

namespace ACE
{
class CPreferencesDialog : public CEditorDialog
{
	Q_OBJECT

public:

	CPreferencesDialog(QWidget* pParent);

signals:

	void ImplementationSettingsAboutToChange();
	void ImplementationSettingsChanged();
	void EnableSaveButton(bool);

private:

	QString m_projectPath;
	bool    m_saveButtonEnable = false;
};
} // namespace ACE
