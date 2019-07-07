// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractCommentItem.h"
#include "AbstractNodeGraphViewModel.h"

namespace CryGraphEditor {

CAbstractCommentItem::CAbstractCommentItem(CCommentEditorData& data, CNodeGraphViewModel& viewModel)
	: CAbstractNodeGraphViewModelItem(viewModel)
	, m_data(data)
{
	SetAccpetsMoves(true);
	SetAcceptsGroup(true);
	SetAcceptsSelection(true);
	SetAcceptsTextEditing(true);
	SetAcceptsHighlightning(true);

	m_data.SignalDataChanged.Connect(this, &CAbstractCommentItem::OnDataChanged);
}

CAbstractCommentItem::~CAbstractCommentItem()
{
	m_data.SignalDataChanged.DisconnectObject(this);
}

QPointF CAbstractCommentItem::GetPosition() const
{
	Vec2 pos = m_data.GetPos();
	return QPointF(pos.x, pos.y);
}

void CAbstractCommentItem::SetPosition(QPointF position)
{
	Vec2 pos(position.x(), position.y());
	if (GetAcceptsMoves() && m_data.GetPos() != pos)
	{
		m_data.SetPos(pos);
		SignalPositionChanged();
	}
}

void CAbstractCommentItem::Serialize(Serialization::IArchive& archive)
{
	archive(m_data.GetText(), "text", "Text");

	if (archive.isInput())
	{
		SignalTextChanged();
	}
}

void CAbstractCommentItem::OnDataChanged()
{
	SignalTextChanged();
	SignalPositionChanged();
}

}
