// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_GraphViewWidget.h"

#include "Schematyc_PropertiesWidget.h"

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
	return new CPropertiesWidget(selectedItems);
}

/*
   QWidget* CGraphView::CreatePropertiesWidget(CryGraphEditor::GraphItemSet& selectedItems)
   {

   }

   bool CGraphView::PopulateNodeContextMenu(CryGraphEditor::CAbstractNodeItem& node, QMenu& menu)
   {

   }*/

}
