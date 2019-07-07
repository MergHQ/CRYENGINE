// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <QPointer>
#include "QRollupBar.h"
#include "IToolWindowArea.h"

class QToolWindowManager;
class QLabel;

class QTOOLWINDOWMANAGER_EXPORT QToolWindowRollupBarArea : public QRollupBar, public IToolWindowArea
{
	Q_OBJECT;
	Q_INTERFACES(IToolWindowArea);

public:
	explicit QToolWindowRollupBarArea(QToolWindowManager* manager, QWidget* parent = nullptr);
	virtual ~QToolWindowRollupBarArea();

	virtual void                addToolWindow(QWidget* toolWindow, int index = -1) override;
	virtual void                addToolWindows(const QList<QWidget*>& toolWindows, int index = -1) override;

	virtual void                removeToolWindow(QWidget* toolWindow) override;

	virtual QList<QWidget*>     toolWindows() override;

	virtual QVariantMap         saveState() override;
	virtual void                restoreState(const QVariantMap& data, int stateFormat) override;
	virtual void                adjustDragVisuals() override;

	virtual QWidget*            getWidget() override { return this; }

	virtual bool                switchAutoHide(bool newValue) override;

	virtual const QPalette&     palette() const override                        { return QRollupBar::palette(); }
	virtual void                clear() override                                { QRollupBar::clear(); }
	virtual QRect               rect() const override                           { return QRollupBar::rect(); }
	virtual QSize               size() const override                           { return QRollupBar::size(); }
	virtual int                 count() const override                          { return QRollupBar::count(); }
	virtual QWidget*            widget(int index) const override                { return QRollupBar::widget(index); }
	virtual void                deleteLater() override                          { QRollupBar::deleteLater(); }
	virtual int                 width() const override                          { return QRollupBar::width(); }
	virtual int                 height() const override                         { return QRollupBar::height(); }
	virtual const QRect         geometry() const override                       { return QRollupBar::geometry(); }
	virtual void                hide() override                                 { QRollupBar::hide(); }
	virtual QObject*            parent() const override                         { return QRollupBar::parent(); }
	virtual void                setParent(QWidget* parent) override             { QRollupBar::setParent(parent); }
	virtual int                 indexOf(QWidget* w) const override;
	virtual QWidget*            parentWidget() const override                   { return QRollupBar::parentWidget(); }
	virtual QPoint              mapFromGlobal(const QPoint& pos) const override { return QRollupBar::mapFromGlobal(pos); }
	virtual QPoint              mapToGlobal(const QPoint& pos) const override   { return QRollupBar::mapToGlobal(pos); }
	virtual void                setCurrentWidget(QWidget* w) override;

	virtual QPoint              mapCombineDropAreaFromGlobal(const QPoint& pos) const override;
	virtual QRect               combineAreaRect() const override;
	virtual QRect               combineSubWidgetRect(int index) const override;
	virtual int                 subWidgetAt(const QPoint& pos) const override;
	virtual QTWMWrapperAreaType areaType() const override { return QTWMWrapperAreaType::watRollups; }
protected:
	virtual bool                eventFilter(QObject* o, QEvent* ev) override;
protected Q_SLOTS:
	void                        closeRollup(int index);
	virtual void                mouseReleaseEvent(QMouseEvent* e) override;
	void                        swapToRollup();
private:
	void                        setDraggable(bool draggable);

	QPointer<QToolWindowManager> m_manager;
	QLabel*                      m_pTopWidget;
	QPoint                       m_areaDragStart;
	bool                         m_areaDraggable;
	bool                         m_tabDragCanStart;
	bool                         m_areaDragCanStart;
};
