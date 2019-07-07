// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Notifications/NotificationCenter.h"
#include "QtUtil.h"
#include "IEditorImpl.h"
#include <BoostPythonMacros.h>
#include <Util/FileUtil.h>

#include <CrySystem/ICryLink.h>

#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QPixmap>
#include <QScreen>
#include <QTime>
#include <QWindow>

namespace Private_ShortcutExporter
{

void PyTakeScreenshot()
{
	QScreen* pScreen = nullptr;
	QWindow* pWindow = QApplication::focusWindow();
	if (pWindow)
		pScreen = pWindow->screen();
	if (!pScreen)
		pScreen = QGuiApplication::primaryScreen();

	QString path = QtUtil::GetAppDataFolder() + "/Screenshots/";
	QDir().mkdir(path);
	QString date = QDate::currentDate().toString() + " (" + QTime::currentTime().toString() + ")";

	date.replace(':', '-').replace(' ', '_');
	path += "Sandbox " + date + ".png";
	path = CFileUtil::addUniqueSuffix(path);

	QPixmap picture = pScreen->grabWindow(pWindow->winId());
	QFile file(path);

	file.open(QIODevice::WriteOnly);
	picture.save(&file, "PNG");

	string showInExplorer = CryLinkService::CCryLinkUriFactory::GetUriV("Editor", "general.show_in_explorer '%s'", path.toUtf8().constData());
	GetIEditorImpl()->GetNotificationCenter()->ShowInfo("Screenshot", QString("The Screenshot was saved.") + showInExplorer.c_str());
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTakeScreenshot, general, take_screenshot,
                                     "Saves the screenshot of the currently active window.",
                                     "general.take_screenshot()");

REGISTER_EDITOR_COMMAND_SHORTCUT(general, take_screenshot, CKeyboardShortcut("F12"));

}
