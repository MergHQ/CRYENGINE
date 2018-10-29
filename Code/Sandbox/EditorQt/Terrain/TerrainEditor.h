// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorFramework/Editor.h"

class QTabWidget;

class CTerrainEditor : public CDockableEditor
{
public:
	CTerrainEditor(QWidget* parent = nullptr);

	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	virtual QRect                             GetPaneRect() override               { return QRect(0, 0, 800, 500); }

	virtual const char*                       GetEditorName() const override       { return "Terrain Editor"; }
	void                                      InitTerrainMenu();

	virtual void                              SetLayout(const QVariantMap& state);
	virtual QVariantMap                       GetLayout() const override;

protected:
	virtual void customEvent(QEvent* pEvent) override;

private:
	QTabWidget* m_pTabWidget;
	int         m_sculptTabIdx;
	int         m_paintTabIdx;
};
