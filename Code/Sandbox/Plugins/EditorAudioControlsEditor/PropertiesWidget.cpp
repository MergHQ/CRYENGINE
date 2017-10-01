// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PropertiesWidget.h"

#include "AudioAssetsManager.h"
#include "ConnectionsWidget.h"
#include "SystemControlsEditorIcons.h"

#include <ACETypes.h>
#include <IEditor.h>
#include <QtUtil.h>
#include <Serialization/QPropertyTree/QPropertyTree.h>

#include <QLabel>
#include <QString>
#include <QVBoxLayout>

using namespace QtUtil;

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CPropertiesWidget::CPropertiesWidget(CAudioAssetsManager* pAssetsManager)
	: m_pAssetsManager(pAssetsManager)
	, m_pPropertyTree(new QPropertyTree())
	, m_pConnectionsWidget(new CConnectionsWidget())
{
	CRY_ASSERT(m_pAssetsManager);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QVBoxLayout* const pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);

	m_pPropertyTree->setSizeToContent(true);
	pMainLayout->addWidget(m_pPropertyTree);

	m_pUsageHint = std::make_unique<QString>(tr("Select an audio control from the left pane to see its properties!"));

	m_pConnectionsLabel = new QLabel(*m_pUsageHint);
	m_pConnectionsLabel->setAlignment(Qt::AlignCenter);
	m_pConnectionsLabel->setWordWrap(true);
	pMainLayout->addWidget(m_pConnectionsLabel);
	
	pMainLayout->addWidget(m_pConnectionsWidget);

	auto revertFunction = [&]()
	{
		if (!m_supressUpdates)
		{
			m_pPropertyTree->revert();
		}
	};

	pAssetsManager->signalItemAdded.Connect(revertFunction, reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalItemRemoved.Connect(revertFunction, reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalControlModified.Connect(revertFunction, reinterpret_cast<uintptr_t>(this));

	connect(m_pPropertyTree, &QPropertyTree::signalAboutToSerialize, [&]() { m_supressUpdates = true; });
	connect(m_pPropertyTree, &QPropertyTree::signalSerialized, [&]() { m_supressUpdates = false; });
}

//////////////////////////////////////////////////////////////////////////
CPropertiesWidget::~CPropertiesWidget()
{
	m_pAssetsManager->signalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalControlModified.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalControlModified.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CPropertiesWidget::Reload()
{
	m_pConnectionsWidget->Reload();
}

//////////////////////////////////////////////////////////////////////////
void CPropertiesWidget::SetSelectedControls(std::vector<CAudioControl*> const& selectedControls)
{
	// Update property tree
	m_pPropertyTree->detach();
	Serialization::SStructs serializers;

	for (auto const pControl : selectedControls)
	{
		CRY_ASSERT(pControl != nullptr);
		serializers.emplace_back(*pControl);
	}

	m_pPropertyTree->attach(serializers);

	// Update connections
	if (selectedControls.size() == 1)
	{
		CAudioControl* const pControl = selectedControls[0];

		if (pControl->GetType() != EItemType::Switch)
		{
			m_pConnectionsWidget->SetControl(pControl);
			m_pConnectionsWidget->setHidden(false);
			m_pConnectionsLabel->setAlignment(Qt::AlignLeft);
			m_pConnectionsLabel->setText(tr("Connections"));
		}
		else
		{
			m_pConnectionsWidget->setHidden(true);
			m_pConnectionsLabel->setAlignment(Qt::AlignCenter);
			m_pConnectionsLabel->setText(tr("Select a switch state to see its connections!"));
		}
	}
	else
	{
		m_pConnectionsWidget->setHidden(true);
		m_pConnectionsWidget->SetControl(nullptr);
		m_pConnectionsLabel->setAlignment(Qt::AlignCenter);
		m_pConnectionsLabel->setText(*m_pUsageHint);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropertiesWidget::BackupTreeViewStates()
{
	m_pConnectionsWidget->BackupTreeViewStates();
}

//////////////////////////////////////////////////////////////////////////
void CPropertiesWidget::RestoreTreeViewStates()
{
	m_pConnectionsWidget->RestoreTreeViewStates();
}
} // namespace ACE
