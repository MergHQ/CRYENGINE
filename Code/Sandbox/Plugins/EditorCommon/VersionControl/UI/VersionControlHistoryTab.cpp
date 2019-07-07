// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlHistoryTab.h"
#include <QLabel>
#include <QVBoxLayout>

CVersionControlHistoryTab::CVersionControlHistoryTab(QWidget* pParent /*= nullptr*/)
{
	auto pLabel = new QLabel();
	pLabel->setText("Coming soon");
	pLabel->setAlignment(Qt::AlignCenter);
	auto pLayout = new QVBoxLayout();
	pLayout->addWidget(pLabel);
	setLayout(pLayout);
}
