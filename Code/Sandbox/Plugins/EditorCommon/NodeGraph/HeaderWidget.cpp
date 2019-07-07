// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "HeaderWidget.h"

#include "HeaderWidgetStyle.h"
#include "NodeWidget.h"
#include "NodeWidgetStyle.h"
#include "HeaderIconWidget.h"
#include "TextWidget.h"
#include "NodeGraphView.h"
#include "TextWidgetStyle.h"
#include "AbstractNodeItem.h"
#include "AbstractGroupItem.h"
#include "NodeGraphViewGraphicsWidget.h"

#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>
#include <QGraphicsScene>

namespace CryGraphEditor {

class CHeaderTypeIcon : public CHeaderIconWidget
{
public:
	CHeaderTypeIcon(CNodeGraphViewGraphicsWidget& viewWidget)
		: CHeaderIconWidget(viewWidget)
		, m_style(viewWidget.GetStyle().GetHeaderWidgetStyle())
	{
		SetDisplayIcon(&m_style.GetNodeIconViewPixmap(StyleState_Default));
	}

	CHeaderTypeIcon::~CHeaderTypeIcon()
	{
	}

private:
	const CHeaderWidgetStyle& m_style;
};

CHeaderWidget::CHeaderWidget(CNodeGraphViewGraphicsWidget& viewWidget, bool showTypeIcon)
	: m_viewWidget(viewWidget)
	, m_name(viewWidget, *this)
	, m_view(viewWidget.GetView())
{
	m_pStyle = &(viewWidget.GetStyle().GetHeaderWidgetStyle());

	const qreal height = m_pStyle->GetHeight();
	setMaximumHeight(height);
	setMinimumHeight(height);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	CAbstractNodeGraphViewModelItem* pItem = viewWidget.GetAbstractItem(); 
	if (CAbstractNodeItem* pNodeItem = pItem->Cast<CAbstractNodeItem>())
	{
		pNodeItem->SignalNameChanged.Connect(this, &CHeaderWidget::OnNameChanged);
		m_name.SignalTextChanged.Connect([pNodeItem](const QString& text) { pNodeItem->SetName(text); });
	}
	else if (CAbstractGroupItem* pGroupItem = pItem->Cast<CAbstractGroupItem>())
	{
		pGroupItem->SignalNameChanged.Connect(this, &CHeaderWidget::OnNameChanged);
		m_name.SignalTextChanged.Connect([pGroupItem](const QString& name) { pGroupItem->SetName(name); });
	}

	m_name.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_name.setMaximumHeight(height - 2);
	
	const QMargins hm = m_pStyle->GetMargins();
	m_pLayout = new QGraphicsLinearLayout();
	m_pLayout->setOrientation(Qt::Horizontal);
	m_pLayout->setContentsMargins(hm.left(), hm.top(), hm.right(), hm.bottom());
	m_pLayout->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

	m_pLayout->addItem(&m_name);
	m_pLayout->setAlignment(&m_name, Qt::AlignLeft);
	m_pLayout->addStretch(hm.left());

	if (showTypeIcon)
	{
		CHeaderTypeIcon* pHeaderIcon = new CHeaderTypeIcon(viewWidget);
		AddIcon(pHeaderIcon, EIconSlot::Left);
	}
	setLayout(m_pLayout);

	// Note: We need to set the name after the layout was set because otherwise the width
	// requirement calculation is wrong.
	CRY_ASSERT_MESSAGE(m_viewWidget.GetAbstractItem(), "View widget abstract item should not be null.");
	SetName(m_viewWidget.GetAbstractItem()->GetName());
}

QString CHeaderWidget::GetName() const
{
	return m_name.GetText();
}

void CHeaderWidget::SetName(QString name)
{
	QFontMetrics fontMetrics = QFontMetrics(m_viewWidget.GetStyle().GetHeaderTextStyle().GetTextFont());

	m_name.SetText(name);
	m_name.setMinimumWidth(fontMetrics.width(name));
}

void CHeaderWidget::EditName()
{
	m_name.TriggerEdit();
}

void CHeaderWidget::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
	const bool wasAntialisingSet = pPainter->renderHints().testFlag(QPainter::Antialiasing);
	pPainter->setRenderHints(QPainter::Antialiasing, true);

	const QRectF geom = boundingRect().adjusted(0, 0, 1, 1);
	if (m_viewWidget.IsSelected())
	{
		const QColor selectionColor = m_pStyle->GetSelectionColor();
		pPainter->setBrush(QBrush(selectionColor));
	}
	else if (m_viewWidget.IsDeactivated())
	{
		// TODO: Deactivation color!
		pPainter->setBrush(QBrush(QColor(94, 94, 94)));
		// ~TODO
	}
	else
	{
		const float hc = m_pStyle->GetHeight() / 2;
		QLinearGradient headerGradient(QPointF(0, hc), QPointF(m_viewWidget.size().width(), hc));
		headerGradient.setColorAt(0, m_pStyle->GetLeftColor());
		headerGradient.setColorAt(1, m_pStyle->GetRightColor());

		pPainter->setBrush(QBrush(headerGradient));
	}

	pPainter->setPen(Qt::PenStyle::NoPen);
	pPainter->drawRect(geom);

	if (wasAntialisingSet)
		pPainter->setRenderHints(QPainter::Antialiasing, false);
}

void CHeaderWidget::AddIcon(CHeaderIconWidget* pHeaderIcon, EIconSlot slot)
{
	const QMargins hm = m_pStyle->GetMargins();

	if (slot == EIconSlot::Left)
	{
		//if (hm.left() > 0)
		//	m_pLayout->insertStretch(0, hm.left());

		m_pLayout->insertItem(0, pHeaderIcon);
		m_pLayout->setAlignment(pHeaderIcon, Qt::AlignVCenter | Qt::AlignLeft);
	}
	else
	{
		m_pLayout->addItem(pHeaderIcon);
		m_pLayout->setAlignment(pHeaderIcon, Qt::AlignVCenter | Qt::AlignRight);

		if (hm.right() > 0)
			m_pLayout->addStretch(hm.right());
	}

	m_pLayout->activate();
}

void CHeaderWidget::SetNameWidth(int32 width)
{
	m_name.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_name.setPreferredWidth(width);
}

void CHeaderWidget::SetNameColor(QColor color)
{
	m_name.SetTextColor(color);
}

void CHeaderWidget::OnNameChanged()
{
	if (CAbstractNodeItem* pNodeItem = m_viewWidget.GetAbstractItem()->Cast<CAbstractNodeItem>())
	{
		SetName(pNodeItem->GetName());
	}
	else if (CAbstractGroupItem* pGroupItem = m_viewWidget.GetAbstractItem()->Cast<CAbstractGroupItem>())
	{
		SetName(QString(pGroupItem->GetEditorData().GetName().c_str()));
	}
}

void CHeaderWidget::OnSelectionChanged(bool isSelected)
{
	m_name.SetSelectionStyle(isSelected);
}

}
