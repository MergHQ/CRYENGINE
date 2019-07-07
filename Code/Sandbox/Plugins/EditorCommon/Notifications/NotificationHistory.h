// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <QWidget>

class CNotificationModel;
class CNotificationFilterModel;
class QAdvancedTreeView;
class QToolButton;

//! Shows a complete history of all notifications (except for progress/tasks)

class CNotificationHistory : public QWidget
{
public:
	CNotificationHistory(QWidget* pParent = nullptr);

protected:
	void OnNotificationsAdded();
	void OnContextMenu(const QPoint& pos) const;
	void OnDoubleClicked(const QModelIndex& index);

	void LoadState();
	void SaveState();

protected:
	QAdvancedTreeView*        m_pTreeView;
	CNotificationFilterModel* m_pFilterModel;
	CNotificationModel*       m_pNotificationModel;
	QToolButton*              m_pShowInfo;
	QToolButton*              m_pShowWarnings;
	QToolButton*              m_pShowErrors;
};
