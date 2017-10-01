// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <ACETypes.h>

class QToolButton;
class QVBoxLayout;

namespace ACE
{
class CMiddlewareDataFilterProxyModel;
class CMiddlewareDataModel;
class CAudioTreeView;
class CElidedLabel;

class CMiddlewareDataWidget final : public QWidget
{
	Q_OBJECT

public:

	CMiddlewareDataWidget();
	virtual ~CMiddlewareDataWidget() override;

	void Reset();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

private slots:

	void OnContextMenu(QPoint const& pos) const;

private:

	void InitFilterWidgets(QVBoxLayout* const pMainLayout);

	CMiddlewareDataFilterProxyModel* const m_pFilterProxyModel;
	CMiddlewareDataModel* const            m_pAssetsModel;
	CElidedLabel* const                    m_pImplNameLabel;
	QToolButton* const                     m_pHideAssignedButton;
	CAudioTreeView* const                  m_pTreeView;
	QString                                m_filter;
};
} // namespace ACE
