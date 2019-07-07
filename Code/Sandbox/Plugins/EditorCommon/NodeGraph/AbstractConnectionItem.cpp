// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractConnectionItem.h"

#include "AbstractPinItem.h"
#include "NodeGraphViewStyle.h"

#include <QColor>

namespace CryGraphEditor {

CAbstractConnectionItem::CAbstractConnectionItem(CNodeGraphViewModel& viewModel)
	: CAbstractNodeGraphViewModelItem(viewModel)
	, m_pSourcePin(nullptr)
	, m_pTargetPin(nullptr)
	, m_isDeactivated(false)
{
	SetAcceptsSelection(true);
	SetAcceptsHighlightning(true);
	SetAcceptsDeactivation(true);
}

CAbstractConnectionItem::CAbstractConnectionItem(CAbstractPinItem& sourcePin, CAbstractPinItem& targetPin, CNodeGraphViewModel& viewModel)
	: CAbstractNodeGraphViewModelItem(viewModel)
	, m_pSourcePin(&sourcePin)
	, m_pTargetPin(&targetPin)
	, m_isDeactivated(false)
{
	SetAcceptsSelection(true);
	SetAcceptsHighlightning(true);
	SetAcceptsDeactivation(true);

	m_pSourcePin->AddConnection(*this);
	m_pTargetPin->AddConnection(*this);
}

CAbstractConnectionItem::~CAbstractConnectionItem()
{
	if (m_pSourcePin)
		m_pSourcePin->RemoveConnection(*this);
	if (m_pTargetPin)
		m_pTargetPin->RemoveConnection(*this);

	m_pSourcePin = m_pTargetPin = nullptr;
}

void CAbstractConnectionItem::SetDeactivated(bool isDeactivated)
{
	if (GetAcceptsDeactivation() && m_isDeactivated != isDeactivated)
	{
		m_isDeactivated = isDeactivated;
		SignalDeactivatedChanged();
	}
}

}
