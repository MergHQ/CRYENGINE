// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractNodeItem.h"
#include "AbstractPinItem.h"
#include "AbstractNodeGraphViewModel.h"
#include "NodeGraphViewStyle.h"

namespace CryGraphEditor {

CAbstractNodeItem::CAbstractNodeItem(CNodeEditorData& data, CNodeGraphViewModel& viewModel)
	: CAbstractNodeGraphViewModelItem(viewModel)
	, m_isDeactivated(false)
	, m_hasErrors(false)
	, m_hasWarnings(false)
	, m_data(data)
{
	SetAccpetsMoves(true);
	SetAcceptsGroup(true);
	SetAcceptsSelection(true);
	SetAcceptsHighlightning(true);
	SetAcceptsDeactivation(true);

	m_data.SignalDataChanged.Connect(this, &CAbstractNodeItem::OnDataChanged);
}

CAbstractNodeItem::~CAbstractNodeItem()
{
	m_data.SignalDataChanged.DisconnectObject(this);
}

void CAbstractNodeItem::SetName(const QString& name)
{
	if (GetAcceptsRenaming() && m_name != name)
	{
		m_name = name;
		SignalNameChanged();
	}
}

QPointF CAbstractNodeItem::GetPosition() const
{
	Vec2 pos = m_data.GetPos();
	return QPointF(pos.x, pos.y);
}

void CAbstractNodeItem::SetPosition(QPointF position)
{
	Vec2 pos(position.x(), position.y());
	if (GetAcceptsMoves() && m_data.GetPos() != pos)
	{
		m_data.SetPos(pos);
		SignalPositionChanged();
	}
}

void CAbstractNodeItem::SetDeactivated(bool isDeactivated)
{
	if (GetAcceptsDeactivation() && m_isDeactivated != isDeactivated)
	{
		m_isDeactivated = isDeactivated;
		SignalDeactivatedChanged(isDeactivated);
	}
}

void CAbstractNodeItem::SetWarnings(bool hasWarnings)
{
	m_hasWarnings = hasWarnings;
}

void CAbstractNodeItem::SetErrors(bool hasErrors)
{
	m_hasErrors = hasErrors;
}

CAbstractPinItem* CAbstractNodeItem::GetPinItemById(QVariant id) const
{
	for (CAbstractPinItem* pPinItem : GetPinItems())
	{
		if (pPinItem->HasId(id))
			return pPinItem;
	}

	return nullptr;
}

CAbstractPinItem* CAbstractNodeItem::GetPinItemByIndex(uint32 index) const
{
	const PinItemArray& pins = GetPinItems();
	if (index < pins.size())
	{
		return pins[index];
	}

	return nullptr;
}

uint32 CAbstractNodeItem::GetPinItemIndex(const CAbstractPinItem& pin) const
{
	uint32 index = 0;
	for (CAbstractPinItem* pPinItem : GetPinItems())
	{
		if (pPinItem == &pin)
			return index;
		++index;
	}

	return 0xffffffff;
}

void CAbstractNodeItem::OnDataChanged()
{
	SignalPositionChanged();
}

}
