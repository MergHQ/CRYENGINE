// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controls/EditorDialog.h"

namespace ACE
{
class CPreferencesDialog final : public CEditorDialog
{
	Q_OBJECT

public:

	CPreferencesDialog(QWidget* const pParent);

signals:

	void SignalImplementationSettingsAboutToChange();
	void SignalImplementationSettingsChanged();
	void SignalEnableSaveButton(bool);

private:

	QString m_projectPath;
	bool    m_saveButtonEnable = false;
};
} // namespace ACE
