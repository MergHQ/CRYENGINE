// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <QWidget>

class CVersionControlPendingChangesWidget;
class QPushButton;

//! Widget for displaying workspace overview that includes pending changes and possibility to submit and revert.
class EDITOR_COMMON_API CVersionControlWorkspaceOverviewTab : public QWidget
{
	Q_OBJECT
public:
	CVersionControlWorkspaceOverviewTab(QWidget* pParent = nullptr);
	~CVersionControlWorkspaceOverviewTab();

private:
	//! Triggered when "Submit..." button is pressed. Which open submission popup.
	void OnSubmit();
	//! Triggered when "Revert..." button is pressed. Which ask first for confirmation.
	void OnRevert();

	void UpdateButtonsStates();

	QPushButton* m_pSubmitButton{ nullptr };
	QPushButton* m_pRevertButton{ nullptr };
	CVersionControlPendingChangesWidget* m_pPendingChangesWidget{ nullptr };
};
