// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <stdio.h>

class CMonoDevelopManager
{
public:
	static CMonoDevelopManager* GetInstance();

	void                        Register();
	void                        Unregister();
private:
	CMonoDevelopManager(void);
	~CMonoDevelopManager(void);

	static void          COMMAND_ReloadGame();
	static void          COMMAND_OpenMonoDevelop();

	static void CALLBACK OnMonoDevelopExit(void* context, BOOLEAN isTimeOut);

	static CMonoDevelopManager* g_pInstance;
	HANDLE                      m_pMonoDevelopWait;
	HANDLE                      m_pMonoDevelopProcess;
};

