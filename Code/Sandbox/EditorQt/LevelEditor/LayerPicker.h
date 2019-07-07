// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "Controls/EditorDialog.h"

class CObjectLayer;
class QAdvancedTreeView;
class QItemSelection;
class QModelIndex;

//TODO : does not allow to select folders.
class CLayerPicker : public CEditorDialog
{
public:
	CLayerPicker();

	void          SetSelectedLayer(CObjectLayer* layer);
	CObjectLayer* GetSelectedLayer() const { return m_selectedLayer; }

protected:
	virtual QSize sizeHint() const override { return QSize(400, 500); }

private:
	void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void OnOk();
	void SelectLayerIndex(const QModelIndex& index);

	CObjectLayer*      m_selectedLayer;
	QAdvancedTreeView* m_treeView;
	QPushButton*       m_pOkButton;
};
