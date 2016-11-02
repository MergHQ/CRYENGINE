// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QObject>
#include <QMap>
#include <QList>

// Only enable this functionality if on Windows, and if not requested disabled
#if (defined(_WIN32) || defined(_WIN64)) && !defined(QTWM_NO_TASKBAR_THUMBNAILS) && QT_VERSION >= 0x050000
#define QTWM_CREATE_TASKBAR_THUMBNAILS
#endif

#include "QToolWindowManagerCommon.h"

#ifdef QTWM_CREATE_TASKBAR_THUMBNAILS
#include <windows.h>
#include <QAbstractNativeEventFilter>

struct ITaskbarList3;
class QToolWindowManager;
class QTOOLWINDOWMANAGER_EXPORT QToolWindowTaskbarHandler : public QObject, QAbstractNativeEventFilter
{
	Q_OBJECT;
public:
	// Constructs the instance. *Must* be called at the start of the manager.
	static void Create(QToolWindowManager* manager) { m_instance = new QToolWindowTaskbarHandler(manager); }
	// Static wrapper methods to forward calls to the instance
	static void Register(QWidget* widget) { m_instance->DoRegister(widget); }
	static void Unregister(QWidget* widget) { m_instance->DoUnregister(widget); }
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return m_instance->WindowProc(hwnd, uMsg, wParam, lParam); }

private slots:
	// Notification that a window's title or icon needs updating
	void onTitleChange();
	void onIconChange();
	void requestThumbnailRefresh();

private:
	QToolWindowTaskbarHandler(QToolWindowManager* manager);
	~QToolWindowTaskbarHandler();
	Q_DISABLE_COPY(QToolWindowTaskbarHandler)
	
	virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
	bool ensureTaskbar();
	void DoRegister(QWidget* widget);
	void DoUnregister(QWidget* widget);
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	QMap<HWND, QWidget*> m_hwndMap;
	QMap<QWidget*, HWND> m_reverseMap;
	QList<QWidget*> m_registerQueue;
	static QToolWindowTaskbarHandler* m_instance;
	ITaskbarList3* m_pTaskbarList;
	WNDCLASSEX m_wx;
	ATOM m_atom;
	UINT m_taskBarCreatedId;
	QToolWindowManager* m_manager;
	QWidget* m_mainWindow;

	// DWM library
	HMODULE m_dwm;
	// Pointers to DWM functions
	typedef HRESULT (WINAPI *dwmInvalidateIconicBitmaps_t)(HWND hwnd);
	dwmInvalidateIconicBitmaps_t dwmInvalidateIconicBitmaps;
	typedef HRESULT (WINAPI *dwmSetIconicThumbnail_t)(HWND hwnd, HBITMAP hbmp, DWORD dwSITFlags);
	dwmSetIconicThumbnail_t dwmSetIconicThumbnail;
	typedef HRESULT (WINAPI *dwmSetIconicLivePreviewBitmap_t)(HWND hwnd, HBITMAP hbmp, POINT* pptClient, DWORD dwSITFlags);
	dwmSetIconicLivePreviewBitmap_t dwmSetIconicLivePreviewBitmap;
	typedef HRESULT (WINAPI *dwmSetWindowAttribute_t)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
	dwmSetWindowAttribute_t dwmSetWindowAttribute;
};
#else
// moc doesn't pick up the class unless an empty one is defined here
class QTOOLWINDOWMANAGER_EXPORT QToolWindowTaskbarHandler : public QObject
{
	Q_OBJECT;
	private slots:
		void onTitleChange() {};
		void onIconChange() {};
		void requestThumbnailRefresh() {};
};
#endif
