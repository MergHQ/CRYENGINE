// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <SystemTypes.h>

class QPropertyTree;
class QAttributeFilterProxyModel;

namespace ACE
{
class CSystemControl;
class CConnectionModel;
class CTreeView;

class CConnectionsWidget final : public QWidget
{
	Q_OBJECT

public:

	CConnectionsWidget(QWidget* const pParent);
	virtual ~CConnectionsWidget() override;

	void SetControl(CSystemControl* const pControl, bool const restoreSelection);
	void Reload();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

signals:

	void SignalSelectConnectedImplItem(CID const itemId);

private slots:

	void OnContextMenu(QPoint const& pos);
	void OnConnectionAdded(CID const id);

private:

	// QObject
	bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	// ~QObject

	void RemoveSelectedConnection();
	void RefreshConnectionProperties();
	void UpdateSelectedConnections();

	CSystemControl*                   m_pControl;
	QPropertyTree* const              m_pConnectionProperties;
	QAttributeFilterProxyModel* const m_pAttributeFilterProxyModel;
	CConnectionModel* const           m_pConnectionModel;
	CTreeView* const                  m_pTreeView;
	int const                         m_nameColumn;
};
} // namespace ACE
