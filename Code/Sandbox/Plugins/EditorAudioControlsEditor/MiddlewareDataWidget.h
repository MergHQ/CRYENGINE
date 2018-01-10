// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <SystemTypes.h>

class QFilteringPanel;

namespace ACE
{
class CSystemAssetsManager;
class CSystemControl;
class CMiddlewareDataModel;
class CMiddlewareFilterProxyModel;
class CTreeView;

class CMiddlewareDataWidget final : public QWidget
{
	Q_OBJECT

public:

	CMiddlewareDataWidget(CSystemAssetsManager* const pAssetsManager, QWidget* const pParent);
	virtual ~CMiddlewareDataWidget() override;

	void Reset();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();
	void SelectConnectedImplItem(CID const itemId);

signals:

	void SignalSelectConnectedSystemControl(CSystemControl& sytemControl, CID const itemId);

private slots:

	void OnContextMenu(QPoint const& pos);

private:

	void ClearFilters();

	CSystemAssetsManager* const        m_pAssetsManager;
	CMiddlewareFilterProxyModel* const m_pMiddlewareFilterProxyModel;
	CMiddlewareDataModel* const        m_pMiddlewareDataModel;
	QFilteringPanel*                   m_pFilteringPanel;
	CTreeView* const                   m_pTreeView;
	int const                          m_nameColumn;
};
} // namespace ACE
