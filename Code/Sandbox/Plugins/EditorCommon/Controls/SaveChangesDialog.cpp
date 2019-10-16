// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SaveChangesDialog.h"

#include "CryIcon.h"

#include <QAbstractButton>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

CSaveChangesDialog::CSaveChangesDialog()
	: CEditorDialog(QStringLiteral("SaveChangesDialog"), nullptr, false)
	, m_buttonPressed(QDialogButtonBox::StandardButton::Cancel)
{
	setWindowTitle(tr("Unsaved changes"));

	const int iconSize = 48;
	m_pIconLabel = new QLabel(this);
	m_pIconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pIconLabel->setPixmap(CryIcon("icons:Dialogs/dialog-warning.ico").pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On));

	m_pSummaryLabel = new QLabel(this);

	QScrollArea* pScrollArea = new QScrollArea(this);
	m_pFileListLabel = new QLabel(pScrollArea);
	pScrollArea->setWidget(m_pFileListLabel);

	m_pButtons = new QDialogButtonBox(this);
	m_pButtons->setStandardButtons(QDialogButtonBox::Discard | QDialogButtonBox::Cancel);

	QVBoxLayout* pMainLayout = new QVBoxLayout(this);
	QHBoxLayout* pUpperLayout = new QHBoxLayout();
	pUpperLayout->addWidget(m_pSummaryLabel);
	pMainLayout->addLayout(pUpperLayout);

	QHBoxLayout* pMidLayout = new QHBoxLayout;
	pMidLayout->addWidget(m_pIconLabel);
	pMidLayout->addWidget(pScrollArea);
	pMainLayout->addLayout(pMidLayout);

	pMainLayout->addWidget(m_pButtons, 0, Qt::AlignRight);
	pMainLayout->setSpacing(20);

	connect(m_pButtons, &QDialogButtonBox::clicked, this, &CSaveChangesDialog::ButtonClicked);

	SetResizable(true);
	resize(500, 270);
}

void CSaveChangesDialog::AddChangeList(const string& name, const std::vector<string>& changes)
{
	m_changeLists.emplace_back(name, changes);
}

void CSaveChangesDialog::FillText()
{
	int totalChanges = 0;
	QString warningBodyText;
	for (const auto& changeList : m_changeLists)
	{
		warningBodyText += changeList.m_name + "\n";
		for (const auto& file : changeList.m_changes)
		{
			warningBodyText.append(QString("    %1\n").arg(file.c_str()));
			++totalChanges;
		}
		warningBodyText += "\n";
	}

	QString headerText = QString("There are %1 unsaved changes of the following files:").arg(totalChanges);
	m_pSummaryLabel->setText(headerText);

	m_pFileListLabel->setText(warningBodyText);
	m_pFileListLabel->adjustSize();
}

void CSaveChangesDialog::ButtonClicked(QAbstractButton* button)
{
	m_buttonPressed = m_pButtons->standardButton(button);
	accept();
}

QDialogButtonBox::StandardButton CSaveChangesDialog::GetButton() const
{
	return m_buttonPressed;
}

int CSaveChangesDialog::Execute()
{
	FillText();
	return exec();
}
