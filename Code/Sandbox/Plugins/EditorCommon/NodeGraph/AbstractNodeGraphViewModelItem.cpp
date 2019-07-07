// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractNodeGraphViewModelItem.h"

#include "AbstractGroupItem.h"
#include "NodeGraphItemPropertiesWidget.h"

namespace CryGraphEditor {

CAbstractNodeGraphViewModelItem::CAbstractNodeGraphViewModelItem(CNodeGraphViewModel& viewModel)
	: m_viewModel(viewModel)
	, m_propertiesPriority(0)
	, m_acceptsGroup(false)
	, m_acceptsMoves(false)
	, m_acceptsSelection(false)
	, m_acceptsHighlightning(false)
	, m_acceptsDeactivation(false)
	, m_allowsMultiItemProperties(false)
	, m_acceptsRenaming(false)
	, m_acceptsDeletion(true)
	, m_acceptsCopy(true)
	, m_acceptsPaste(true)
	, m_acceptsTextEditing(false)
{
}

CAbstractNodeGraphViewModelItem::~CAbstractNodeGraphViewModelItem()
{
	SignalDeletion(this);
}

}
