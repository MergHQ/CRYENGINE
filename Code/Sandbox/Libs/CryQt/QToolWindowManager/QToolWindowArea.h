// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IToolWindowArea.h"
#include "QToolWindowManagerCommon.h"
#include "QToolWindowTabBar.h"

#include <QFrame>
#include <QPointer>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>

class QGridLayout;
class QLabel;
class QToolWindowManager;

class QTOOLWINDOWMANAGER_EXPORT QToolWindowSingleTabAreaFrame : public QFrame
{
	Q_OBJECT;
public:
	QToolWindowSingleTabAreaFrame(QToolWindowManager* manager, QWidget* parent);
	virtual ~QToolWindowSingleTabAreaFrame() {}
	QWidget*     contents() { return m_pContents; }
	virtual void setContents(QWidget* widget);

	void         setCloseButtonVisible(bool bVisible);

protected:
	QGridLayout* m_pLayout;
	QLabel*      m_pCaption;
	QWidget*     m_pContents;

private slots:
	void         closeWidget();
private:
	virtual void closeEvent(QCloseEvent* e) override;
	QIcon        getCloseButtonIcon() const;

	QToolWindowManager* m_manager;
	QPushButton*        m_closeButton;
	friend class QToolWindowArea;
};

class QTOOLWINDOWMANAGER_EXPORT QToolWindowArea : public QTabWidget, public IToolWindowArea
{
	Q_OBJECT;
	Q_INTERFACES(IToolWindowArea);
public:
	explicit QToolWindowArea(QToolWindowManager* manager, QWidget* parent = 0);
	virtual ~QToolWindowArea();

	virtual void            addToolWindow(QWidget* toolWindow, int index = -1) override;
	virtual void            addToolWindows(const QList<QWidget*>& toolWindows, int index = -1) override;

	virtual void            removeToolWindow(QWidget* toolWindow) override;

	virtual QList<QWidget*> toolWindows() override;

	virtual QVariantMap     saveState() override;
	virtual void            restoreState(const QVariantMap& data, int stateFormat) override;
	virtual void            adjustDragVisuals() override;

	virtual QWidget*        getWidget() override { return this; }

	virtual bool            switchAutoHide(bool newValue) override;

	QToolWindowTabBar*      tabBar() const { return (QToolWindowTabBar*)QTabWidget::tabBar(); }

	//QTabWidget members
	virtual const QPalette&     palette() const override                        { return QTabWidget::palette(); }
	virtual void                clear() override                                { QTabWidget::clear(); }
	virtual QRect               rect() const override                           { return QTabWidget::rect(); }
	virtual QSize               size() const override                           { return QTabWidget::size(); }
	virtual int                 count() const override                          { return QTabWidget::count(); }
	virtual QWidget*            widget(int index) const override                { return QTabWidget::widget(index); }
	virtual void                deleteLater() override                          { QTabWidget::deleteLater(); }
	virtual int                 width() const override                          { return QTabWidget::width(); }
	virtual int                 height() const override                         { return QTabWidget::height(); }
	virtual const QRect         geometry() const override                       { return QTabWidget::geometry(); }
	virtual void                hide() override                                 { QTabWidget::hide(); }
	virtual QObject*            parent() const override                         { return QTabWidget::parent(); }
	virtual void                setParent(QWidget* parent) override             { QTabWidget::setParent(parent); }
	virtual int                 indexOf(QWidget* w) const override;
	virtual QWidget*            parentWidget() const override                   { return QTabWidget::parentWidget(); }
	virtual QPoint              mapFromGlobal(const QPoint& pos) const override { return QTabWidget::mapFromGlobal(pos); }
	virtual QPoint              mapToGlobal(const QPoint& pos) const override   { return QTabWidget::mapToGlobal(pos); }
	virtual void                setCurrentWidget(QWidget* w) override;
	//QTabBar wrappers
	virtual QPoint              mapCombineDropAreaFromGlobal(const QPoint& pos) const override { return tabBar()->mapFromGlobal(pos); }
	virtual QRect               combineAreaRect() const override;
	virtual QRect               combineSubWidgetRect(int index) const override                 { return tabBar()->tabRect(index); }
	virtual int                 subWidgetAt(const QPoint& pos) const override                  { return tabBar()->tabAt(pos); }
	virtual QTWMWrapperAreaType areaType() const override                                      { return QTWMWrapperAreaType::watTabs; }

protected:
	virtual bool                eventFilter(QObject* o, QEvent* ev) override;
	virtual bool                shouldShowSingleTabFrame();
	virtual bool                event(QEvent* event) override;

	QToolWindowSingleTabAreaFrame* m_pTabFrame;
	QPointer<QToolWindowManager>   m_manager;

protected Q_SLOTS:
	void tabCloseButtonClicked();
	void closeTab(int index);
	void showContextMenu(const QPoint& point);
	void swapToRollup();

private:
	QPushButton* createCloseButton();
	virtual void mouseReleaseEvent(QMouseEvent* e) override;

	bool m_tabDragCanStart;
	bool m_areaDragCanStart;
	bool m_useCustomTabCloseButton;
	friend class QToolWindowSingleTabAreaFrame;
};
