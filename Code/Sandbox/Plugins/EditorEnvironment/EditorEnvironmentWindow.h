// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditor.h>
#include <EditorFramework/Editor.h>
#include <QtViewPane.h>

class QPresetsWidget;
class QTimeOfDayWidget;

class CEditorEnvironmentWindow : public CDockableEditor, public IEditorNotifyListener
{
	Q_OBJECT
public:
	CEditorEnvironmentWindow();
	~CEditorEnvironmentWindow();

private:
	const char* GetEditorName() const override { return "Environment Editor"; }
	void        OnEditorNotifyEvent(EEditorNotifyEvent event) override;
	void        customEvent(QEvent* event) override;

	QWidget*    CreateToolbar();
	void        RestorePersonalizationState();
	void        SavePersonalizationState() const;

	QPresetsWidget*   m_presetsWidget;
	QTimeOfDayWidget* m_timeOfDayWidget;
	QDockWidget*      m_pPresetsDock;
};
