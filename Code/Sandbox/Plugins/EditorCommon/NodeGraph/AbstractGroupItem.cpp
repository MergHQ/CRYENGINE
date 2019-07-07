// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractGroupItem.h"

#include "NodeWidget.h"
#include "GroupWidget.h"
#include "AbstractNodeItem.h"
#include "AbstractNodeGraphViewModel.h"

namespace CryGraphEditor {

CAbstractGroupItem::CAbstractGroupItem(CGroupEditorData& data, CNodeGraphViewModel& viewModel)
	: CAbstractNodeGraphViewModelItem(viewModel)
	, m_data(data)
{
	SetAccpetsMoves(true);
	SetAcceptsSelection(true);
	SetAcceptsHighlightning(true);

	m_data.SignalDataChanged.Connect(this, &CAbstractGroupItem::OnDataChanged);
	SignalPositionChanged.Connect(this, &CAbstractGroupItem::UpdateLinkedItemsPositions);
}

CAbstractGroupItem::~CAbstractGroupItem()
{
	SignalPositionChanged.DisconnectObject(this);
	m_data.SignalDataChanged.DisconnectObject(this);
}

QString CAbstractGroupItem::GetName() const
{
	return QString(m_data.GetName());
}

void CAbstractGroupItem::SetName(const QString& name)
{
	if (GetAcceptsRenaming() && (QString(m_data.GetName()) != name))
	{
		m_data.SetName(name.toStdString().c_str());
		SignalNameChanged();
	}
}

QPointF CAbstractGroupItem::GetPosition() const
{
	Vec2 pos = m_data.GetPos();
	return QPointF(pos.x, pos.y);
}

void CAbstractGroupItem::SetPosition(QPointF position)
{
	Vec2 pos(position.x(), position.y());
	if (GetAcceptsMoves() && m_data.GetPos() != pos)
	{
		m_data.SetPos(pos);
		SignalPositionChanged();
	}
}

void CAbstractGroupItem::LinkItem(CAbstractNodeGraphViewModelItem& item)
{
	CRY_ASSERT(item.GetAcceptsGroup() == true);
	
	item.SetAcceptsGroup(false);

	CGroupItems& items = GetItems();
	CryGUID      guid  = item.GetId().value<CryGUID>();
	QPointF      pivot = item.GetPosition() - GetPosition();

	items[guid].m_pivot = Vec2(pivot.x(), pivot.y());
}

void CAbstractGroupItem::UnlinkItem(CAbstractNodeGraphViewModelItem& item)
{
	auto& items = GetItems();

	if (item.GetAcceptsGroup() == false)
	{
		item.SetAcceptsGroup(true);
		items.erase(item.GetId().value<CryGUID>());
	}
}

void CAbstractGroupItem::OnDataChanged()
{
	SignalNameChanged();
	SignalAABBChanged();
	SignalPositionChanged();
	SignalItemsChanged(*this);
}

void CAbstractGroupItem::UpdateLinkedItemsPositions()
{
	for (auto& it : m_data.GetItems())
	{
		auto pItem = GetViewModel().GetAbstractModelItemById(QVariant::fromValue(it.first));
		if (pItem)
		{
			Vec2 pos = it.second.m_pivot + m_data.GetPos();
			pItem->SetPosition(QPointF(pos.x, pos.y));
		}
	}
}

}