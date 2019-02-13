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

	void                                      RegisterDockingWidgets();
	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	virtual QRect                             GetPaneRect() override               { return QRect(0, 0, 500, 800); }

	virtual const char*                       GetEditorName() const override       { return "Level Settings"; }
	void                                      InitMenu();

private:
	virtual void CreateDefaultLayout(CDockableContainer* pSender) override;
};
