// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QAdvancedTreeView.h"

class CLayer;
class CTerrainManager;

class QTerrainLayerView : public QAdvancedTreeView
{
	Q_OBJECT
public:
	QTerrainLayerView(QWidget* pParent, CTerrainManager* pTerrainManager);
	~QTerrainLayerView();

private:
	virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
	virtual void customEvent(QEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;

	void         SelectRow(int row);
	void         SelectedLayerChanged(CLayer* pLayer);
	void         LayersChanged();

	QAbstractItemModel* m_pModel;
	CTerrainManager*    m_pTerrainManager;
	bool                m_selectionProcessing;
};
