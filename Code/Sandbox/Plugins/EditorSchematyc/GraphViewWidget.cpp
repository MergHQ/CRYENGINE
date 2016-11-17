// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GraphViewWidget.h"

#include "PropertiesWidget.h"
#include "GraphViewModel.h"

namespace CrySchematycEditor {

CNodeGraphView::CNodeGraphView()
	: CryGraphEditor::CNodeGraphView()
{

}

CNodeGraphView::~CNodeGraphView()
{

}

QWidget* CNodeGraphView::CreatePropertiesWidget(CryGraphEditor::GraphItemSet& selectedItems)
{
	CPropertiesWidget* pPropertiesWidget = new CPropertiesWidget(selectedItems);

	if (CNodeGraphViewModel* pModel = static_cast<CNodeGraphViewModel*>(GetModel()))
	{
		QObject::connect(pPropertiesWidget, &CPropertiesWidget::SignalPropertyChanged, pModel, &CNodeGraphViewModel::Refresh);
	}
	return pPropertiesWidget;
}

}
