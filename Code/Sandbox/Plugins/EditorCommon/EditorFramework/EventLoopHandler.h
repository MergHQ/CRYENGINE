// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"

#include <QAbstractNativeEventFilter>
#include <QObject>

#include <functional>
#include <vector>

class CEditor;

class EDITOR_COMMON_API CEventLoopHandler : public QObject, public QAbstractNativeEventFilter
{
	Q_OBJECT
public:
	CEventLoopHandler();
	~CEventLoopHandler();

	void SetDefaultHandler(QWidget* pDefaultHandler) { m_pDefaultHandler = pDefaultHandler; }
	void AddNativeHandler(uintptr_t id, std::function<bool(void*, long*)>);
	void RemoveNativeHandler(uintptr_t id);

private:
	struct CallBack
	{
		CallBack(size_t id, std::function<bool(void*, long*)> cb)
			: m_id(id)
			, m_cb(cb)
		{}

		size_t                            m_id;
		std::function<bool(void*, long*)> m_cb;
	};

	virtual bool eventFilter(QObject* object, QEvent* event) override;
	virtual bool nativeEventFilter(const QByteArray& eventType, void* message, long*) override;

	QWidget*              m_pDefaultHandler;
	CEditor*              m_pFocusedEditor;
	std::vector<CallBack> m_nativeListeners;
};
