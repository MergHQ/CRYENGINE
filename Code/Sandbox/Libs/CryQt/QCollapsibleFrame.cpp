// Copyright 2001-2016 Crytek GmbH. All rights reserved.
#include <StdAfx.h>
#include "QCollapsibleFrame.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyleOption>
#include <QEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QApplication>
#include <QDrag>
#include <QToolButton>
#include <qtabbar.h>


struct QCollapsibleFrame::SImplementation
{
	SImplementation(QCollapsibleFrame* pCollapsibleWidget, const QString& title);

	void SetWidget(QWidget* pWidget);
	void SetTitle(const QString& title);
	void SetClosable(bool closable);
	bool Closable() const;

	QCollapsibleFrame*       m_pCollapsibleWidget;
	QFrame*					 m_pContentsFrame;
	CCollapsibleFrameHeader* m_pHeaderWidget;
	QWidget*                 m_pWidget;
};

struct CCollapsibleFrameHeader::SImplementation
{
	SImplementation(CCollapsibleFrameHeader* pHeaderWidget, const QString& title, QCollapsibleFrame* pParentCollapsible);

	QSize GetIconSize() const;

	void  SetupCollapseButton();
	void  SetTitle(const QString& title);
	void  SetIconSize(const QSize& iconSize);
	void  SetupMainLayout();
	void  OnCollapseButtonClick();
	void  SetClosable(bool closable);
	bool  Closable() const;

	CCollapsibleFrameHeader*     m_pHeader;
	QCollapsibleFrame*           m_pParentCollapsible;
	QLabel*                      m_pTitleLabel;
	QLabel*                      m_pIconLabel;
	QPushButton*                 m_pCollapseButton;
	QToolButton*				 m_pCloseButton;
	QSize						 m_iconSize;
	QIcon						 m_collapsedIcon;
	QIcon						 m_expandedIcon;
	bool                         m_bCollapsed;
	static const CryIconColorMap s_colorMap;
};

// removes dark tinting
const CryIconColorMap CCollapsibleFrameHeader::SImplementation::s_colorMap = []
{
	CryIconColorMap colorMap;
	colorMap[QIcon::Mode::Normal] = QColor(255, 255, 255);
	return colorMap;
} ();

QCollapsibleFrame::SImplementation::SImplementation(QCollapsibleFrame* pCollapsibleWidget, const QString& title)
	: m_pCollapsibleWidget(pCollapsibleWidget)
	, m_pWidget(nullptr)
{
	m_pHeaderWidget = new CCollapsibleFrameHeader(title, m_pCollapsibleWidget);
	auto pMainLayout = new QVBoxLayout;
	pMainLayout->setSpacing(0);
	pMainLayout->setContentsMargins(2, 2, 2, 2);
	m_pCollapsibleWidget->setLayout(pMainLayout);

	pMainLayout->addWidget(m_pHeaderWidget);

	m_pContentsFrame = new QFrame(m_pCollapsibleWidget);
	auto frameLayout = new QVBoxLayout();
	frameLayout->setSpacing(0);
	frameLayout->setContentsMargins(2, 2, 2, 2);
	m_pContentsFrame->setLayout(frameLayout);
	pMainLayout->addWidget(m_pContentsFrame);


}

void QCollapsibleFrame::SImplementation::SetWidget(QWidget* pWidget)
{
	auto pMainLayout = m_pContentsFrame->layout();

	// remove old widget
	if (m_pWidget)
	{
		pMainLayout->removeWidget(m_pWidget);
	}

	m_pWidget = pWidget;
	if (m_pWidget)
	{
		pMainLayout->addWidget(m_pWidget);
		m_pWidget->setHidden(m_pHeaderWidget->m_pImpl->m_bCollapsed);
	}
}

void QCollapsibleFrame::SImplementation::SetTitle(const QString& title)
{
	m_pHeaderWidget->m_pImpl->SetTitle(title);
}

bool QCollapsibleFrame::SImplementation::Closable() const
{
	return m_pHeaderWidget->Closable();
}

void QCollapsibleFrame::SImplementation::SetClosable(bool closable)
{
	m_pHeaderWidget->SetClosable(closable);
}

QCollapsibleFrame::QCollapsibleFrame(const QString& title, QWidget* pParent)
	: QWidget(pParent)
	, m_pImpl(new SImplementation(this, title))
{
	connect(m_pImpl->m_pHeaderWidget->m_pImpl->m_pCloseButton, SIGNAL(clicked()), this,  SLOT(OnCloseRequested()));
}

QCollapsibleFrame::~QCollapsibleFrame()
{}

QWidget* QCollapsibleFrame::GetWidget() const
{
	return m_pImpl->m_pWidget;
}

QWidget * QCollapsibleFrame::GetDragHandler() const
{
	return m_pImpl->m_pHeaderWidget->m_pImpl->m_pCollapseButton;
}

void QCollapsibleFrame::SetWidget(QWidget* pWidget)
{
	m_pImpl->SetWidget(pWidget);
}

void QCollapsibleFrame::SetClosable(bool closable)
{
	m_pImpl->SetClosable(closable);
}

bool QCollapsibleFrame::Closable() const
{
	return m_pImpl->Closable();
}

void QCollapsibleFrame::SetTitle(const QString& title)
{
	m_pImpl->SetTitle(title);
}

void QCollapsibleFrame::SetCollapsed(bool collapsed)
{
	if (collapsed != m_pImpl->m_pHeaderWidget->m_pImpl->m_bCollapsed)
	{
		m_pImpl->m_pHeaderWidget->OnCollapseButtonClick();
	}
}

bool QCollapsibleFrame::Collapsed() const
{
	return m_pImpl->m_pHeaderWidget->m_pImpl->m_bCollapsed;
}

void QCollapsibleFrame::paintEvent(QPaintEvent*)
{
	QStyleOption opt;
	opt.init(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}


void QCollapsibleFrame::OnCloseRequested()
{
	CloseRequested(this);
}

CCollapsibleFrameHeader::SImplementation::SImplementation(CCollapsibleFrameHeader* pHeaderWidget, const QString& title, QCollapsibleFrame* pParentCollapsible)
	: m_pHeader(pHeaderWidget)
	, m_iconSize(16, 16)
	, m_pParentCollapsible(pParentCollapsible)
	, m_pTitleLabel(new QLabel(title))
	, m_bCollapsed(false)
{
	QPixmap collapsedPixmap("icons:Window/Collapse_Arrow_Right_Tinted.ico");
	if (collapsedPixmap.isNull())
	{
		m_collapsedIcon = QApplication::style()->standardIcon(QStyle::SP_TitleBarShadeButton);
	}
	else
	{
		m_collapsedIcon = CryIcon(collapsedPixmap, s_colorMap);
	}
	QPixmap expandedPixmap("icons:Window/Collapse_Arrow_Down_Tinted.ico");
	if (expandedPixmap.isNull())
	{
		m_expandedIcon = QApplication::style()->standardIcon(QStyle::SP_TitleBarUnshadeButton);
	}
	else
	{
		m_expandedIcon = CryIcon(expandedPixmap, s_colorMap);
	}
	SetupCollapseButton();
	SetupMainLayout();
}

QSize CCollapsibleFrameHeader::SImplementation::GetIconSize() const
{
	return m_iconSize;
}

void CCollapsibleFrameHeader::SImplementation::SetupCollapseButton()
{
	m_pCollapseButton = new QPushButton(m_pParentCollapsible);
	m_pCollapseButton->setObjectName("CollapseButton");
	
	QHBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(6, 2, 6, 2);
	m_pCollapseButton->setLayout(layout);
	
	m_pIconLabel = new QLabel(m_pCollapseButton);
	m_pIconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	
	m_pTitleLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	
	m_pCloseButton = new QToolButton(m_pCollapseButton);
	m_pCloseButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
	m_pCloseButton->setFixedHeight(16);
	m_pCloseButton->setFixedWidth(16);

	m_pCloseButton->setVisible(false);
	m_pIconLabel->setPixmap(m_expandedIcon.pixmap(m_expandedIcon.actualSize(m_iconSize)));
	
	layout->addWidget(m_pIconLabel);
	layout->addWidget(m_pTitleLabel);
	layout->addStretch();
	layout->addWidget(m_pCloseButton);
}

void CCollapsibleFrameHeader::SImplementation::SetTitle(const QString& title)
{
	m_pTitleLabel->setText(title);
}

void CCollapsibleFrameHeader::SImplementation::SetIconSize(const QSize& iconSize)
{
	m_iconSize = iconSize;
}

void CCollapsibleFrameHeader::SImplementation::SetupMainLayout()
{
	auto pTitleLayout = new QHBoxLayout;
	pTitleLayout->setContentsMargins(0, 0, 0, 0);
	pTitleLayout->setSpacing(0);
	pTitleLayout->addWidget(m_pCollapseButton);
	m_pHeader->setLayout(pTitleLayout);
}

void CCollapsibleFrameHeader::SImplementation::OnCollapseButtonClick()
{
	m_bCollapsed = !m_bCollapsed;

	if (m_bCollapsed)
	{
		m_pIconLabel->setPixmap(m_collapsedIcon.pixmap(m_collapsedIcon.actualSize(m_iconSize)));
	}
	else
	{
		m_pIconLabel->setPixmap(m_expandedIcon.pixmap(m_expandedIcon.actualSize(m_iconSize)));
	}

	auto pWidget = m_pParentCollapsible->m_pImpl->m_pWidget;
	if (pWidget)
	{
		pWidget->setHidden(m_bCollapsed);
	}

}

void CCollapsibleFrameHeader::SImplementation::SetClosable(bool closable)
{
	m_pCloseButton->setVisible(closable);
}

bool CCollapsibleFrameHeader::SImplementation::Closable() const
{
	return m_pCloseButton->isVisible();
}

CCollapsibleFrameHeader::CCollapsibleFrameHeader(const QString& title, QCollapsibleFrame* pParentCollapsible)
	: QWidget(pParentCollapsible)
	, m_pImpl(new SImplementation(this, title, pParentCollapsible))
{
	// do not install in SImplementation constructor because events may
	// be fired and eventFilter needs m_pImpl to be fully constructed
	m_pImpl->m_pCollapseButton->installEventFilter(this);
	connect(m_pImpl->m_pCollapseButton, SIGNAL(clicked()), this, SLOT(OnCollapseButtonClick()));
}

QSize CCollapsibleFrameHeader::GetIconSize() const
{
	return m_pImpl->GetIconSize();
}

void CCollapsibleFrameHeader::SetIconSize(const QSize& iconSize)
{
	m_pImpl->SetIconSize(iconSize);
}

void CCollapsibleFrameHeader::SetClosable(bool closable)
{
	m_pImpl->SetClosable(closable);
}

bool CCollapsibleFrameHeader::Closable() const
{
	return m_pImpl->Closable();
}

void CCollapsibleFrameHeader::paintEvent(QPaintEvent*)
{
	QStyleOption opt;
	opt.init(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}

bool CCollapsibleFrameHeader::eventFilter(QObject* pWatched, QEvent* pEvent)
{
	auto pCollapseButton = m_pImpl->m_pCollapseButton;
	// can only be disabled when parent is disabled. cannot be explicitly disabled from outside this
	// class due to no public access.
	if ((pEvent->type() != QEvent::MouseButtonRelease) || (pWatched != pCollapseButton) || isEnabled())
	{
		return QWidget::eventFilter(pWatched, pEvent);
	}
	m_pImpl->OnCollapseButtonClick();
	return true;
}

void CCollapsibleFrameHeader::OnCollapseButtonClick()
{
	m_pImpl->OnCollapseButtonClick();
}
