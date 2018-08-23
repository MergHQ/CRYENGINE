// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>
#include "QMainFrameMenuBar.h"

// Qt
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStyleOption>
#include <QPainter>
#include <QStandardItemModel>

//Editor
#include "Commands/CommandModel.h"
#include "Commands/QCommandAction.h"
#include "QtUtil.h"

QMainFrameMenuBar::QMainFrameMenuBar(QMenuBar* pMenuBar /* = 0*/, QWidget* pParent /* = 0*/)
	: QWidget(pParent)
	, m_pMenuBar(pMenuBar)
{
	QHBoxLayout* pMainLayout = new QHBoxLayout();
	pMainLayout->setMargin(0);
	pMainLayout->setSpacing(0);

	if (pMenuBar)
	{
		pMainLayout->addWidget(pMenuBar);
	}

	// Add tray area to the menu bar
	pMainLayout->addWidget(GetIEditorImpl()->GetTrayArea());

	setLayout(pMainLayout);
}

void QMainFrameMenuBar::paintEvent(QPaintEvent* pEvent)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
