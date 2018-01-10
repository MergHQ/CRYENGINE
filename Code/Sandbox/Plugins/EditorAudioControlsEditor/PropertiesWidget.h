// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <SystemTypes.h>

class QPropertyTree;
class QLabel;
class QString;

namespace ACE
{
class CConnectionsWidget;
class CSystemAssetsManager;
class CSystemAsset;

class CPropertiesWidget final : public QWidget
{
	Q_OBJECT

public:

	explicit CPropertiesWidget(CSystemAssetsManager* const pAssetsManager, QWidget* const pParent);
	virtual ~CPropertiesWidget() override;

	void Reload();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

signals:

	void SignalSelectConnectedImplItem(CID const itemId);

public slots:

	void OnSetSelectedAssets(std::vector<CSystemAsset*> const& selectedAssets);

private:

	void RevertPropertyTree();

	CSystemAssetsManager* const m_pAssetsManager;
	CConnectionsWidget* const   m_pConnectionsWidget;
	QPropertyTree* const        m_pPropertyTree;
	QLabel*                     m_pConnectionsLabel;
	std::unique_ptr<QString>    m_pUsageHint;
	bool                        m_supressUpdates = false;
};
} // namespace ACE
