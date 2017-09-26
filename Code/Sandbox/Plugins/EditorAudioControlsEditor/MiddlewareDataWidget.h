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

	void SetAllowedControls(EItemType const type, bool const bAllowed);
	void Reset();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

private slots:

	void OnContextMenu(QPoint const& pos) const;

private:

	void InitFilterWidgets(QVBoxLayout* pMainLayout);

	bool                             m_allowedATLTypes[static_cast<int>(EItemType::NumTypes)]; // Replace this. Possibly with a QList.
	CMiddlewareDataFilterProxyModel* m_pFilterProxyModel;
	CMiddlewareDataModel*            m_pAssetsModel;
	CElidedLabel*                    m_pImplNameLabel;
	QToolButton*                     m_pHideAssignedButton;
	CAudioTreeView*                  m_pTreeView;
	QString                          m_filter;
};
} // namespace ACE
