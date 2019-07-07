// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <EditorFramework/Editor.h>

#include <QWidget>

#include <memory>

class CVegetationEditor : public CDockableEditor
{
public:
	explicit CVegetationEditor(QWidget* parent = nullptr);
	~CVegetationEditor();

private:
	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	virtual QRect                             GetPaneRect() override               { return QRect(0, 0, 800, 500); }
	virtual const char*                       GetEditorName() const override       { return "Vegetation Editor"; }

	void                                      RegisterActions();
	void                                      OnDelete();

private:
	struct SImplementation;
	std::unique_ptr<SImplementation> m_pImpl;
};
