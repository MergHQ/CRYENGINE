// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QWidget>
#include <QVariantMap>

#include "QToolWindowManagerCommon.h"
#include "IToolWindowWrapper.h"

class QToolWindowManager;

class QTOOLWINDOWMANAGER_EXPORT QToolWindowWrapper : public QWidget, public IToolWindowWrapper
{
	Q_OBJECT;
	Q_INTERFACES(IToolWindowWrapper);
public:
	explicit QToolWindowWrapper(QToolWindowManager* manager, Qt::WindowFlags flags = 0);
	virtual ~QToolWindowWrapper();

	QWidget* getWidget() Q_DECL_OVERRIDE { return this; }
	virtual QWidget* getContents() Q_DECL_OVERRIDE;
	virtual void setContents(QWidget* widget) Q_DECL_OVERRIDE;
	virtual void startDrag() Q_DECL_OVERRIDE;

protected:	
	virtual void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;
	virtual void changeEvent(QEvent *) Q_DECL_OVERRIDE;
	virtual bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;

#if QT_VERSION >= 0x050000
	virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;
#endif
#if (defined(_WIN32) || defined(_WIN64))
	bool winEvent(MSG *msg, long *result);
#endif

private:
	QToolWindowManager* m_manager;
	QWidget* m_contents;
};