// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/NodeGraphView.h>
#include <NodeGraph/NodeWidget.h>

class CAssetNodeBase;
class CDictionaryWidget;
class QPopupWidget;

class CGraphView : public CryGraphEditor::CNodeGraphView
{
public:
	CGraphView(CryGraphEditor::CNodeGraphViewModel* pViewModel);
protected:
	virtual bool PopulateNodeContextMenu(CryGraphEditor::CAbstractNodeItem& node, QMenu& menu) override;
	virtual void ShowGraphContextMenu(QPointF screenPos) override;
	void         OnContextMenuEntryClicked(CAbstractDictionaryEntry& entry);
	virtual void closeEvent(QCloseEvent*pEvent) override;
private:
	std::unique_ptr<QPopupWidget> m_pSearchPopup;
	CDictionaryWidget*            m_pSearchPopupContent;
};

class CAssetWidget : public CryGraphEditor::CNodeWidget
{
public:
	CAssetWidget(CAssetNodeBase& item, CryGraphEditor::CNodeGraphView& view);
protected:
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* pEvent) override;
};
