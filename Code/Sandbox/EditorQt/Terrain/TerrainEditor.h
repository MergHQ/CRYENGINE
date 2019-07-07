// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorFramework/Editor.h"

class QTabWidget;

class CTerrainEditor : public CDockableEditor
{
public:
	CTerrainEditor(QWidget* parent = nullptr);

private:
	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	virtual QRect                             GetPaneRect() override               { return QRect(0, 0, 800, 500); }
	virtual const char*                       GetEditorName() const override       { return "Terrain Editor"; }
	virtual void                              SetLayout(const QVariantMap& state);
	virtual QVariantMap                       GetLayout() const override;
	virtual void                              Initialize() override;

	void                                      RegisterActions();
	void                                      InitTerrainMenu();
	void                                      SetTerrainTool(int tabIndex, CRuntimeClass* pTool);

	QTabWidget* m_pTabWidget;
	int         m_sculptTabIdx;
	int         m_paintTabIdx;
};
