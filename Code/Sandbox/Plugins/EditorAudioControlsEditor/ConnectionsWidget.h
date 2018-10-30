// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/SharedData.h"
#include <QWidget>

class QPropertyTree;
class QAttributeFilterProxyModel;

namespace ACE
{
class CControl;
class CConnectionsModel;
class CTreeView;

class CConnectionsWidget final : public QWidget
{
	Q_OBJECT

public:

	CConnectionsWidget() = delete;
	CConnectionsWidget(CConnectionsWidget const&) = delete;
	CConnectionsWidget(CConnectionsWidget&&) = delete;
	CConnectionsWidget& operator=(CConnectionsWidget const&) = delete;
	CConnectionsWidget& operator=(CConnectionsWidget&&) = delete;

	explicit CConnectionsWidget(QWidget* const pParent);
	virtual ~CConnectionsWidget() override;

	void SetControl(CControl* const pControl, bool const restoreSelection);
	void Reset();
	void OnBeforeReload();
	void OnAfterReload();

private slots:

	void OnContextMenu(QPoint const& pos);
	void OnConnectionAdded(ControlId const id);

private:

	// QObject
	bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	// ~QObject

	void RemoveSelectedConnection();
	void RefreshConnectionProperties();
	void UpdateSelectedConnections();
	void ResizeColumns();

	CControl*                         m_pControl;
	QPropertyTree* const              m_pConnectionProperties;
	QAttributeFilterProxyModel* const m_pAttributeFilterProxyModel;
	CConnectionsModel* const          m_pConnectionModel;
	CTreeView* const                  m_pTreeView;
	int const                         m_nameColumn;
};
} // namespace ACE
