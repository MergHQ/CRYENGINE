// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "QToolWindowManagerCommon.h"

#include <QMenu>
#include <QTabBar>

class QToolButton;
class QToolWindowTabBar;

class QTabSelectionMenu : public QMenu
{
	Q_OBJECT
public:
	QTabSelectionMenu(QToolButton* pToolButton, QToolWindowTabBar* pParentTabbar);

private:
	QToolWindowTabBar* m_pParentTabbar;
};

class QTOOLWINDOWMANAGER_EXPORT QToolWindowTabBar : public QTabBar
{
	Q_OBJECT;
public:
	explicit QToolWindowTabBar(QWidget* parent = 0);

protected:
	void paintEvent(QPaintEvent* e);

private:
	void onSelectionMenuClicked();

	QToolButton*       m_tabSelectionButton;
	QTabSelectionMenu* m_tabSelectionMenu;
};
