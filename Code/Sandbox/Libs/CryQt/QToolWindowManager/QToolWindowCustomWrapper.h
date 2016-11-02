// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "QToolWindowManagerCommon.h"
#include "QCustomWindowFrame.h"
#include "IToolWindowWrapper.h"

#include <QLabel>

#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#include <dwmapi.h>
#pragma warning(disable: 4264)
#pragma warning(disable: 4266)
#endif

class QToolWindowManager;
class QPushButton;
class QToolButton;
class QGridLayout;

class QToolWindowCustomWrapper;

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

	QWidget* getWidget() Q_DECL_OVERRIDE { return this; }
	virtual QWidget* getContents() { return m_contents; }
	virtual void setContents(QWidget* widget) Q_DECL_OVERRIDE { QCustomWindowFrame::setContents(widget,false); }
	virtual void startDrag() Q_DECL_OVERRIDE;

	QRect getWrapperFrameSize();

private:
	void AddToTaskbar();
	void RemoveFromTaskbar();

	virtual bool event(QEvent *)Q_DECL_OVERRIDE;
	virtual void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;
	virtual bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;

protected:
	virtual Qt::WindowFlags calcFrameWindowFlags() Q_DECL_OVERRIDE;

#if QT_VERSION >= 0x050000
	virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;
#endif
#if (defined(_WIN32) || defined(_WIN64))
	bool winEvent(MSG *msg, long *result);
#endif

protected:
	QToolWindowManager* m_manager;

	friend class QToolWindowCustomTitleBar;
};