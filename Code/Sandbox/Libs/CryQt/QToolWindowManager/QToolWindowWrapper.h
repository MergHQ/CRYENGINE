// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IToolWindowWrapper.h"
#include "QToolWindowManagerCommon.h"

#include <QPointer>

class QToolWindowManager;

class QTOOLWINDOWMANAGER_EXPORT QToolWindowWrapper : public QWidget, public IToolWindowWrapper
{
	Q_OBJECT;
	Q_INTERFACES(IToolWindowWrapper);
public:
	explicit QToolWindowWrapper(QToolWindowManager* manager, Qt::WindowFlags flags = 0);
	virtual ~QToolWindowWrapper();

	virtual QWidget* getWidget() override { return this; }
	virtual QWidget* getContents() override;
	virtual void     setContents(QWidget* widget) override;
	virtual void     startDrag() override;
	virtual void     hide() override                     { QWidget::hide(); }
	virtual void     deferDeletion() override;
	virtual void     setParent(QWidget* parent) override { QWidget::setParent(parent); }

protected:
	virtual void closeEvent(QCloseEvent*) override;
	virtual void changeEvent(QEvent*) override;
	virtual bool eventFilter(QObject*, QEvent*) override;
	virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

#if (defined(_WIN32) || defined(_WIN64))
	bool winEvent(MSG* msg, long* result);
#endif

private:
	QPointer<QToolWindowManager> m_manager;
	QWidget*                     m_contents;
};
