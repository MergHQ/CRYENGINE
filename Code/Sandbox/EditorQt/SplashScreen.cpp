// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SplashScreen.h"
#include "Version.h"

#include <QApplication>
#include <QPainter>

SplashScreen* SplashScreen::s_pLogoWindow = nullptr;

SplashScreen::SplashScreen(const QPixmap& pixmap /* = QPixmap() */, Qt::WindowFlags f /* = 0 */)
	: QSplashScreen(pixmap, f)
{
	s_pLogoWindow = this;
	setWindowIcon(QIcon("icons:editor_icon.ico"));
	setWindowTitle("CRYENGINE Sandbox");

	//Should be set after parent construction: it overwrites default Qt behavior
	setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

SplashScreen::~SplashScreen()
{
	s_pLogoWindow = 0;
}

void SplashScreen::SetVersion(const Version& v)
{
	char pVersionText[256];
	cry_sprintf(pVersionText, "Version %d.%d.%d - Build %d", v[3], v[2], v[1], v[0]);
	s_pLogoWindow->version = pVersionText;
	s_pLogoWindow->repaint();
	qApp->processEvents();
}

void SplashScreen::SetInfo(const char* text)
{
	m_text = text;
	update();
	qApp->processEvents();
}

void SplashScreen::SetText(const char* szText)
{
	if (s_pLogoWindow)
	{
		s_pLogoWindow->SetInfo(szText);
	}
}

void SplashScreen::drawContents(QPainter* painter)
{
	QSplashScreen::drawContents(painter);
	painter->setPen(Qt::white);
	painter->drawText(rect().adjusted(30, 5, -5, -42), Qt::AlignLeft | Qt::AlignBottom, version);
	painter->drawText(rect().adjusted(30, 5, -5, -25), Qt::AlignLeft | Qt::AlignBottom, m_text);
}
