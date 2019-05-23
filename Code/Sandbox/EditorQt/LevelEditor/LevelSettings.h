// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorFramework/Editor.h"
#include "QtViewPane.h"
#include "Util/Variable.h"

class QPropertyTree;
class CEnvironmentPresets;

class CLevelSettingsEditor : public CDockableEditor
{
public:
	CLevelSettingsEditor(QWidget* parent = nullptr);

	// CEditor overrides
	virtual void                              Initialize() override;
	virtual const char*                       GetEditorName() const override { return "Level Settings"; }

	void                                      RegisterActions();
	void                                      CreateMenu();
	void                                      RegisterDockingWidgets();
	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	virtual QRect                             GetPaneRect() override               { return QRect(0, 0, 500, 800); }

private:
	// CEditor overrides
	virtual void CreateDefaultLayout(CDockableContainer* pSender) override;
};
