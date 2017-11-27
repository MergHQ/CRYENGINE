// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PropertiesWidget.h"

#include "SystemAssetsManager.h"
#include "ConnectionsWidget.h"

#include <SystemTypes.h>
#include <IEditor.h>
#include <QtUtil.h>
#include <Serialization/QPropertyTree/QPropertyTree.h>

#include <QLabel>
#include <QString>
#include <QVBoxLayout>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CPropertiesWidget::CPropertiesWidget(CSystemAssetsManager* const pAssetsManager, QWidget* const pParent)
	: QWidget(pParent)
	, m_pAssetsManager(pAssetsManager)
	, m_pPropertyTree(new QPropertyTree(this))
	, m_pConnectionsWidget(new CConnectionsWidget(this))
{
	CRY_ASSERT_MESSAGE(m_pAssetsManager != nullptr, "Asset manager is null pointer.");

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QVBoxLayout* const pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);

	m_pPropertyTree->setSizeToContent(true);
	m_pPropertyTree->setExpandLevels(2);
	pMainLayout->addWidget(m_pPropertyTree);

	m_pUsageHint = std::make_unique<QString>(tr("Select an audio control from the left pane to see its properties!"));

	m_pConnectionsLabel = new QLabel(*m_pUsageHint);
	m_pConnectionsLabel->setAlignment(Qt::AlignCenter);
	m_pConnectionsLabel->setWordWrap(true);
	pMainLayout->addWidget(m_pConnectionsLabel);
	
	pMainLayout->addWidget(m_pConnectionsWidget);

	m_pAssetsManager->signalItemAdded.Connect([&]()
	{
		if (!m_pAssetsManager->IsLoading())
		{
			RevertPropertyTree();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalItemRemoved.Connect([&]()
	{
		if (!m_pAssetsManager->IsLoading())
		{
			RevertPropertyTree();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalControlModified.Connect([&]()
	{
		if (!m_pAssetsManager->IsLoading())
		{
			RevertPropertyTree();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalAssetRenamed.Connect([&]()
	{
		if (!m_pAssetsManager->IsLoading())
		{
			RevertPropertyTree();
		}
	}, reinterpret_cast<uintptr_t>(this));

	QObject::connect(m_pPropertyTree, &QPropertyTree::signalAboutToSerialize, [&]() { m_supressUpdates = true; });
	QObject::connect(m_pPropertyTree, &QPropertyTree::signalSerialized, [&]() { m_supressUpdates = false; });
}

//////////////////////////////////////////////////////////////////////////
CPropertiesWidget::~CPropertiesWidget()
{
	m_pAssetsManager->signalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalControlModified.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalAssetRenamed.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CPropertiesWidget::Reload()
{
	m_pConnectionsWidget->Reload();
}

//////////////////////////////////////////////////////////////////////////
void CPropertiesWidget::OnSetSelectedAssets(std::vector<CSystemAsset*> const& selectedAssets)
{
	// Update property tree
	m_pPropertyTree->detach();
	Serialization::SStructs serializers;

	for (auto const pAsset : selectedAssets)
	{
		CRY_ASSERT_MESSAGE(pAsset != nullptr, "Selected asset is null pointer.");
		serializers.emplace_back(*pAsset);
	}

	m_pPropertyTree->attach(serializers);

	// Update connections
	if (selectedAssets.size() == 1)
	{
		ESystemItemType const type = selectedAssets[0]->GetType();

		if ((type != ESystemItemType::Library) && (type != ESystemItemType::Folder))
		{
			CSystemControl* const pControl = static_cast<CSystemControl*>(selectedAssets[0]);

			if (type != ESystemItemType::Switch)
			{
				m_pConnectionsWidget->SetControl(pControl);
				m_pConnectionsWidget->setHidden(false);
				m_pConnectionsLabel->setAlignment(Qt::AlignLeft);
				m_pConnectionsLabel->setText(tr("Connections"));
			}
			else
			{
				m_pConnectionsWidget->setHidden(true);
				m_pConnectionsWidget->SetControl(nullptr);
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

//////////////////////////////////////////////////////////////////////////
void CPropertiesWidget::RevertPropertyTree()
{
	if (!m_supressUpdates)
	{
		m_pPropertyTree->revert();
	}
}
} // namespace ACE
