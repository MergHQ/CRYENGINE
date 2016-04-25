// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "QtUtil.h"
#include "AudioSystemPanel.h"
#include "IAudioSystemEditor.h"
#include "AudioControlsEditorPlugin.h"
#include "AudioSystemModel.h"
#include "QAudioSystemSettingsDialog.h"
#include "ImplementationManager.h"

// Qt
#include <QtUtil.h>
#include <QLineEdit>
#include <QCheckBox>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QToolButton>
#include <QIcon>
#include <QLabel>

namespace ACE
{
CAudioSystemPanel::CAudioSystemPanel()
	: m_pModelProxy(new QAudioSystemModelProxyFilter(this))
	, m_pModel(new QAudioSystemModel())
	, m_pImplNameLabel(nullptr)
{
	resize(299, 674);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QHBoxLayout* pHorizontalLayout = new QHBoxLayout();

	QLineEdit* pNameFilter = new QLineEdit(this);
	pNameFilter->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
	pHorizontalLayout->addWidget(pNameFilter);

	QCheckBox* pHideAssignedCheckbox = new QCheckBox(this);
	pHideAssignedCheckbox->setEnabled(true);
	pHorizontalLayout->addWidget(pHideAssignedCheckbox);

	QToolButton* pSettingsButton = new QToolButton();
	pSettingsButton->setIcon(QIcon("://Icons/Options.ico"));
	connect(pSettingsButton, &QToolButton::clicked, [&]()
		{
			QAudioSystemSettingsDialog dialog(this);
			if (dialog.exec() == QDialog::Accepted)
			{
			  Reset();
			  ImplementationSettingsChanged();
			}
	  });

	pHorizontalLayout->addWidget(pSettingsButton);

	QVBoxLayout* pVerticalLayout = new QVBoxLayout(this);
	pVerticalLayout->setContentsMargins(0, 0, 0, 0);

	m_pImplNameLabel = new QLabel("");
	IAudioSystemEditor* pAudioImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
	if (pAudioImpl)
	{
		m_pImplNameLabel->setText(QtUtil::ToQString(pAudioImpl->GetName()));
	}
	m_pImplNameLabel->setObjectName("ImplementationTitle");
	pVerticalLayout->addWidget(m_pImplNameLabel);

	pVerticalLayout->addLayout(pHorizontalLayout);

	pNameFilter->setPlaceholderText(tr("Search", 0));
	pHideAssignedCheckbox->setText(tr("Hide Assigned", 0));

	m_pModelProxy->setDynamicSortFilter(true);

	connect(pNameFilter, &QLineEdit::textChanged, [&](const QString& filter)
		{
			m_pModelProxy->setFilterFixedString(filter);
	  });

	connect(pHideAssignedCheckbox, &QCheckBox::clicked, [&](bool bHide)
		{
			m_pModelProxy->SetHideConnected(bHide);
	  });

	QTreeView* pTreeView = new QTreeView();
	pTreeView->header()->setVisible(false);
	pTreeView->setDragEnabled(true);
	pTreeView->setDragDropMode(QAbstractItemView::DragOnly);
	pTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

	m_pModelProxy->setSourceModel(m_pModel);
	pTreeView->setModel(m_pModelProxy);
	pVerticalLayout->addWidget(pTreeView);

	// Update the middleware name label.
	// Note the 'this' ptr being passed as a context variable so that Qt can disconnect this lambda when the object is destroyed (ie. the ACE is closed).
	connect(CAudioControlsEditorPlugin::GetImplementationManger(), &CImplementationManager::ImplementationChanged, this, [&]()
		{
			IAudioSystemEditor* pAudioImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
			if (pAudioImpl)
			{
			  m_pImplNameLabel->setText(QtUtil::ToQString(pAudioImpl->GetName()));
			}
	  });
}

void CAudioSystemPanel::SetAllowedControls(EACEControlType type, bool bAllowed)
{
	const ACE::IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
	if (pAudioSystemEditorImpl)
	{
		m_allowedATLTypes[type] = bAllowed;
		uint mask = 0;
		for (int i = 0; i < EACEControlType::eACEControlType_NumTypes; ++i)
		{
			if (m_allowedATLTypes[i])
			{
				mask |= pAudioSystemEditorImpl->GetCompatibleTypes((EACEControlType)i);
			}
		}
		m_pModelProxy->SetAllowedControlsMask(mask);
	}
}

void CAudioSystemPanel::Reset()
{
	m_pModel->Reset();
	m_pModelProxy->invalidate();
}

}
