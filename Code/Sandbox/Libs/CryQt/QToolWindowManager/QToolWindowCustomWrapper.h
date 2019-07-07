// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "QToolWindowManagerCommon.h"
#include "QCustomWindowFrame.h"
#include "IToolWindowWrapper.h"

#include <QLabel>
#include <QPointer>

class QGridLayout;
class QPushButton;
class QToolButton;
class QToolWindowCustomWrapper;
class QToolWindowManager;

class QTOOLWINDOWMANAGER_EXPORT QToolWindowCustomTitleBar : public QCustomTitleBar
{
	Q_OBJECT;
public:
	QToolWindowCustomTitleBar(QToolWindowCustomWrapper* parent);

	friend class QToolWindowCustomWrapper;
};

class QTOOLWINDOWMANAGER_EXPORT QToolWindowCustomWrapper : public QCustomWindowFrame, public IToolWindowWrapper
{
	Q_OBJECT;
	Q_INTERFACES(IToolWindowWrapper);

public:
	explicit QToolWindowCustomWrapper(QToolWindowManager* manager, QWidget* wrappedWidget = nullptr, QVariantMap config = QVariantMap());
	virtual ~QToolWindowCustomWrapper();

	static QToolWindowCustomWrapper* wrapWidget(QWidget* w, QVariantMap config = QVariantMap());

	virtual QWidget*                 getWidget() override                  { return this; }
	virtual QWidget*                 getContents() override                { return m_contents; }
	virtual void                     setContents(QWidget* widget) override { internalSetContents(widget, false); }
	virtual void                     startDrag() override;
	virtual void                     hide() override                       { QCustomWindowFrame::hide(); }
	virtual void                     deferDeletion() override;
	virtual void                     setParent(QWidget* parent) override   { QCustomWindowFrame::setParent(parent); }

private:
	virtual bool event(QEvent*) override;
	virtual void closeEvent(QCloseEvent*) override;

protected:
	virtual Qt::WindowFlags calcFrameWindowFlags() override;
	virtual bool            eventFilter(QObject*, QEvent*) override;
	virtual bool            nativeEvent(const QByteArray& eventType, void* message, long* result) override;

#if (defined(_WIN32) || defined(_WIN64))
	virtual bool winEvent(MSG* msg, long* result);
#endif

protected:
	QPointer<QToolWindowManager> m_manager;

	friend class QToolWindowCustomTitleBar;
};
