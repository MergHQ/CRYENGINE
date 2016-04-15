// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ITestSystem.h> // ITestSystem
#include "Log.h"                   // CLog
#include "UnitTestSystem.h"

// needed for external test application
class CTestSystemLegacy : public ITestSystem
{
public:
	// constructor
	CTestSystemLegacy();
	// destructor
	virtual ~CTestSystemLegacy();

	void Init(IConsole* pConsole);

	// interface ITestSystem -----------------------------------------------

	virtual void                           ApplicationTest(const char* szParam);
	virtual void                           Update();
	virtual void                           BeforeRender();
	virtual void                           AfterRender() {}
	virtual ILog*                          GetILog()     { return m_pLog; }
	virtual void                           Release()     { delete this; }
	virtual void                           SetTimeDemoInfo(STimeDemoInfo* pTimeDemoInfo);
	virtual STimeDemoInfo*                 GetTimeDemoInfo();
	virtual void                           QuitInNSeconds(const float fInNSeconds);

	virtual CryUnitTest::IUnitTestManager* GetIUnitTestManager() { return m_pUnitTestManager; };

private: // --------------------------------------------------------------

	// TGA screenshot
	void        ScreenShot(const char* szDirectory, const char* szFilename);
	//
	void        LogLevelStats();
	// useful when running through a lot of tests
	void        DeactivateCrashDialog();

	static void RunUnitTests(IConsoleCmdArgs* pArgs);

private: // --------------------------------------------------------------

	string                         m_sParameter;      // "" if not in test mode
	CLog*                          m_pLog;            //
	int                            m_iRenderPause;    // counts down every render to delay some processing
	float                          m_fQuitInNSeconds; // <=0 means it's deactivated

	uint32                         m_bFirstUpdate     : 1;
	uint32                         m_bApplicationTest : 1;

	STimeDemoInfo*                 m_pTimeDemoInfo;

	CryUnitTest::CUnitTestManager* m_pUnitTestManager;

	friend class CLevelListener;
};
