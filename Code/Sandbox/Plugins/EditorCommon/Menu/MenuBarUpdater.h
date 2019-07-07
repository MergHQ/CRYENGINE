// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CAbstractMenu;

class QMenuBar;
class QMenu;

class CMenuBarUpdater
{
public:
	CMenuBarUpdater(CAbstractMenu* pAbstractMenu, QMenuBar* pMenuBar);
	~CMenuBarUpdater();

private:
	uintptr_t GetId() const { return (uintptr_t)this; }

private:
	CAbstractMenu* const m_pAbstractMenu;
};

class CMenuUpdater
{
public:
	CMenuUpdater(CAbstractMenu* pAbstractMenu, QMenu* pMenu);
	~CMenuUpdater();

private:
	uintptr_t GetId() const { return (uintptr_t)this; }

private:
	CAbstractMenu* const m_pAbstractMenu;
};