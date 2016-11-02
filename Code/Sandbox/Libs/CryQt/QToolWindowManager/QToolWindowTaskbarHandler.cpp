// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "QToolWindowTaskbarHandler.h"

#ifdef QTWM_CREATE_TASKBAR_THUMBNAILS

#ifdef UNICODE
#define _UNICODE
#endif

#include <QApplication>
#include <QMainWindow>
#include <tchar.h>
#include <dwmapi.h>
#include <ShObjIdl.h>
#include <QTimer>
#include "QToolWindowManager.h"

#define WM_DWMSENDICONICTHUMBNAIL           0x0323
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP   0x0326

// Internal QT conversion methods from pixmaps to Windows bitmap/icon handles
Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap &p, int hbitmapFormat = 0); 
Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &p); 

QToolWindowTaskbarHandler::QToolWindowTaskbarHandler(QToolWindowManager* manager)
{
	m_manager = manager;
	CoInitialize(nullptr);
	m_atom = 0;
	m_dwm = nullptr;
	m_pTaskbarList = nullptr;
	m_taskBarCreatedId = WM_NULL;
	qApp->installNativeEventFilter(this);
}

QToolWindowTaskbarHandler::~QToolWindowTaskbarHandler()
{
	qApp->removeNativeEventFilter(this);
	if (m_dwm)
	{
		FreeLibrary(m_dwm);
	}
	if (m_atom)
	{
		UnregisterClass(m_wx.lpszClassName, m_wx.hInstance);
	}
	if (m_pTaskbarList)
	{
		m_pTaskbarList->Release();		
	}
	CoUninitialize();
}
QToolWindowTaskbarHandler* QToolWindowTaskbarHandler::m_instance = nullptr;

bool QToolWindowTaskbarHandler::nativeEventFilter(const QByteArray &eventType, void *message_, long *result)
{
	// Prepare to work with the taskbar
	if (m_taskBarCreatedId == WM_NULL)
	{
		m_taskBarCreatedId = RegisterWindowMessage(_T("TaskbarButtonCreated")); 
		return false;
	}
	MSG* message = static_cast<MSG*>(message_);
	if (message->message == m_taskBarCreatedId && getMainWindow() && message->hwnd == (HWND)getMainWindow()->winId())
	{
		// Taskbar is ready, get the objects needed for registering windows and perform pending registrations
		m_mainWindow = getMainWindow();
		ensureTaskbar();
		while(!m_registerQueue.empty())
		{
			DoRegister(m_registerQueue.takeFirst());
		}
		qApp->removeNativeEventFilter(this);
		return true;
	}
	return false;
}

bool QToolWindowTaskbarHandler::ensureTaskbar()
{
	if (m_pTaskbarList)
		return true;

	// Register a window class with a custom WindowProc
	memset(&m_wx, 0, sizeof(m_wx));
	m_wx.cbSize = sizeof(WNDCLASSEX);
	m_wx.style = 0;
	m_wx.cbWndExtra = 0;
	m_wx.lpfnWndProc = WndProc;
	m_wx.hInstance = GetModuleHandle(nullptr);
	m_wx.lpszClassName = _T("QTWMTaskbarHandler");
	m_atom = RegisterClassEx(&m_wx);
	if (!m_atom)
	{
		DWORD err = GetLastError();
		char msg[1000];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, &msg[0], 1000, nullptr);
		qWarning("Error %d registering Taskbar handler class: %s", err, msg);
	}

	m_dwm = LoadLibrary(_T("dwmapi.dll"));
	if (!m_dwm)
		return false;
	dwmInvalidateIconicBitmaps = (dwmInvalidateIconicBitmaps_t)GetProcAddress(m_dwm, "DwmInvalidateIconicBitmaps");
	dwmSetIconicLivePreviewBitmap = (dwmSetIconicLivePreviewBitmap_t)GetProcAddress(m_dwm, "DwmSetIconicLivePreviewBitmap");
	dwmSetIconicThumbnail = (dwmSetIconicThumbnail_t)GetProcAddress(m_dwm, "DwmSetIconicThumbnail");
	dwmSetWindowAttribute = (dwmSetWindowAttribute_t)GetProcAddress(m_dwm, "DwmSetWindowAttribute");

	// Get the TaskbarList interface
	HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, (LPVOID*)&m_pTaskbarList);
	if (hr == S_OK)
	{
		hr = m_pTaskbarList->HrInit();
		// Proxies are not asked to refresh their thumbnail automatically; they must inform Windows that they need refreshing. 
		// We set off a timer to do this periodically, to ensure we don't end up with old thumbnails.
		QTimer* timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(requestThumbnailRefresh()));
		timer->start(m_manager->config().value(QTWM_THUMBNAIL_TIMER_INTERVAL, 1000).toInt());
	}
	return hr == S_OK;
}

void QToolWindowTaskbarHandler::requestThumbnailRefresh()
{
	foreach(HWND h, m_hwndMap.keys())
	{
		dwmInvalidateIconicBitmaps(h);
	}
}

void updateWindowIcon(QWidget* sender, HWND h)
{
	// Get the new icons
	QSize bigIconSize(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
	QSize smallIconSize(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
	HICON bigIcon = qt_pixmapToWinHICON(sender->windowIcon().pixmap(bigIconSize));
	HICON smallIcon = qt_pixmapToWinHICON(sender->windowIcon().pixmap(smallIconSize));

	// Update the icons, and free the old ones
	// The new HICONs must not be deleted while they're being used
	bigIcon = (HICON)SendMessage(h, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
	smallIcon = (HICON)SendMessage(h, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
	DeleteObject(bigIcon);
	DeleteObject(smallIcon);
}

void QToolWindowTaskbarHandler::onIconChange()
{
	QWidget* sender = qobject_cast<QWidget*>(QObject::sender());
	HWND h = m_reverseMap.value(sender, nullptr);
	if (!h)
	{
		return;
	}
	updateWindowIcon(sender, h);
}

void updateWindowTitle(QWidget* sender, HWND h)
{
	QString winTitle = sender->windowTitle();
	LPCTSTR windowTitle;
	TCHAR buffer[1000];
#ifndef UNICODE
	// needed for codepage conversion
	QByteArray temp;
#endif
	if (!winTitle.size()) //Qt is using fallback title, get it
	{
		GetWindowText((HWND)sender->winId(), buffer, 1000);
		windowTitle = buffer;
	}
	else
	{
#ifdef UNICODE
		windowTitle = (LPCTSTR)winTitle.utf16();
#else
		temp = winTitle.toLocal8Bit();
		windowTitle = (LPCTSTR)temp.constData();
#endif
	}
	SetWindowText(h, windowTitle);
}

void QToolWindowTaskbarHandler::onTitleChange()
{
	QWidget* sender = qobject_cast<QWidget*>(QObject::sender());
	HWND h = m_reverseMap.value(sender, nullptr);
	if (!h)
	{
		return;
	}
	updateWindowTitle(sender, h);
}

void QToolWindowTaskbarHandler::DoRegister(QWidget* widget)
{
	if (!m_pTaskbarList)
	{
		m_registerQueue.append(widget);
		return;		
	}

	//If we're inside a QMainWindow, we need to create a proxy window to intercept messages
	QMainWindow* mainWindowParent = findClosestParent<QMainWindow*>(widget);
	QWidget* windowWidget = widget;
	while (!windowWidget->isWindow())
	{
		windowWidget = windowWidget->parentWidget();
	}
	bool needsProxy = mainWindowParent == windowWidget;
	HWND h; 

	if (needsProxy)
	{
		// Target the main window from the proxy
		widget = mainWindowParent;
		// Listen for title and icon changes
		connect(mainWindowParent, SIGNAL(windowTitleChanged(const QString &)), this, SLOT(onTitleChange()));
		connect(mainWindowParent, SIGNAL(windowIconChanged(const QIcon &)), this, SLOT(onIconChange()));
		// Create the proxy window
		h = CreateWindowEx(0, m_wx.lpszClassName, nullptr, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
		if (!h)
		{
			DWORD err = GetLastError();
			char msg[1000];
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, &msg[0], 1000, nullptr);
			qWarning("Error %d creating window: %s", err, msg);
			return;
		}
		// Set icon and title
		updateWindowTitle(widget, h);
		updateWindowIcon(widget, h);
		// Tell Windows that we provide previews
		BOOL enable = TRUE;
		dwmSetWindowAttribute(h, DWMWA_FORCE_ICONIC_REPRESENTATION,	&enable, sizeof(enable));
		dwmSetWindowAttribute(h, DWMWA_HAS_ICONIC_BITMAP,	&enable, sizeof(enable));
	}
	else
	{
		// Widgets that are not in a QMainWindow can handle previews automatically
		widget->createWinId();
		h = (HWND)widget->winId();
	}

	// Register the tab with Windows and place it at the end of the tab list
	m_pTaskbarList->RegisterTab(h, (HWND)m_mainWindow->winId());
	m_pTaskbarList->SetTabOrder(h, nullptr);
	// Keep a map from handles to widgets and vice versa, so we can process messages
	m_hwndMap.insert(h, widget);		
	m_reverseMap.insert(widget, h);
}

void QToolWindowTaskbarHandler::DoUnregister(QWidget* widget)
{
	if (m_registerQueue.contains(widget))
	{
		m_registerQueue.removeOne(widget);
	}
	if (!m_hwndMap.values().contains(widget))
		return;
	if (!m_pTaskbarList)
		return;		

	HWND h;
	foreach(h, m_hwndMap.keys())
	{
		if (m_hwndMap[h] == widget)
			break;
	}
	m_reverseMap.remove(widget);
	m_hwndMap.remove(h);
	m_pTaskbarList->UnregisterTab(h);
}

LRESULT QToolWindowTaskbarHandler::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!m_hwndMap.contains(hwnd))
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	QWidget* widget = m_hwndMap[hwnd];

	switch(uMsg)
	{
	case WM_DWMSENDICONICTHUMBNAIL:
		{
			QPixmap thumbnail;
#if QT_VERSION <= 0x050000
			thumbnail = QPixmap::grabWidget(widget);
#else
			thumbnail = widget->grab();
#endif
			// Windows will not accept thumbnails that are too big, so scale down if necessary
			if (HIWORD(lParam) < thumbnail.width() || LOWORD(lParam) < thumbnail.height())
			{
				thumbnail = thumbnail.scaled(HIWORD(lParam), LOWORD(lParam), Qt::KeepAspectRatio);
			}
			HBITMAP hbitmap = qt_pixmapToWinHBITMAP(thumbnail);

			dwmSetIconicThumbnail(hwnd, hbitmap, 0);
			DeleteObject(hbitmap);
			return 0;
		}

	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		{
			QPixmap thumbnail;
#if QT_VERSION <= 0x050000
			thumbnail = QPixmap::grabWidget(widget);
#else
			thumbnail = widget->grab();
#endif

			HBITMAP hbitmap = qt_pixmapToWinHBITMAP(thumbnail);
			dwmSetIconicLivePreviewBitmap(hwnd, hbitmap, 0, 0);
			DeleteObject(hbitmap);
			return 0;
		}

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_ACTIVE)
		{
			if (m_mainWindow->isMinimized())
			{
				m_mainWindow->setWindowState(Qt::WindowNoState);
			}
			widget->activateWindow();
		}
		break;

	case WM_CLOSE:
		DoUnregister(widget);
		widget->close();
		return 0;

	case WM_SYSCOMMAND:
		switch(wParam & 0xFFF0) //lower 4 bits are used internally by Windows
		{
		case SC_MOVE:
		case SC_SIZE:
		case SC_MINIMIZE:
		case SC_MAXIMIZE:
		case SC_RESTORE:
			// forward to widget
			SendMessage((HWND)widget->winId(), WM_SYSCOMMAND, wParam, lParam);
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam); 

}
#endif
