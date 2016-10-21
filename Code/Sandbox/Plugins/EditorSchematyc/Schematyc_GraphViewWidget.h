// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/NodeGraphView.h>

namespace CrySchematycEditor {

class CNodeGraphView : public CryGraphEditor::CNodeGraphView
{
public:
	CNodeGraphView();
	~CNodeGraphView();

	virtual QWidget* CreatePropertiesWidget(CryGraphEditor::GraphItemSet& selectedItems) override;

private:
	// CryGraphEditor::CNodeGraphView
	//virtual QWidget* CreatePropertiesWidget(CryGraphEditor::GraphItemSet& selectedItems) override;
	//virtual bool     PopulateNodeContextMenu(CryGraphEditor::CAbstractNodeItem& node, QMenu& menu) override;
	// ~CryGraphEditor::CNodeGraphView
};

}
