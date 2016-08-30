// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GameStartup.h"
#include <CryCore/Platform/platform_impl.inl>
#include <CryCore/Platform/CryLibrary.h>
#include <CryInput/IHardwareMouse.h>
#include "Game/Game.h"

#include "EditorSpecific/EditorGame.h"

#if ENABLE_AUTO_TESTER
static CAutoTester s_autoTesterSingleton;
#endif

extern "C"
{
	GAME_API IGameStartup* CreateGameStartup()
	{
		static char buff[sizeof(CGameStartup)];
		return new(buff) CGameStartup();
	}

	GAME_API IEditorGame* CreateEditorGame()
	{
		return new CEditorGame();
	}
}

CGameStartup::CGameStartup()
	: m_pGame(nullptr),
	m_gameRef(&m_pGame),
	m_quit(false),
	m_gameDll(0),
	m_pFramework(nullptr),
	m_fullScreenCVarSetup(false)
{
}

CGameStartup::~CGameStartup()
{
	if (m_pGame)
	{
		m_pGame->Shutdown();
		m_pGame = nullptr;
	}

	if (m_gameDll)
	{
		CryFreeLibrary(m_gameDll);
		m_gameDll = 0;
	}

	ShutdownFramework();
}

IGameRef CGameStartup::Init(SSystemInitParams& startupParams)
{
	if (!InitFramework(startupParams))
	{
		return nullptr;
	}

	startupParams.pSystem = GetISystem();
	IGameRef pOut = Reset();

	if (!m_pFramework->CompleteInit())
	{
		pOut->Shutdown();
		return nullptr;
	}

	if (startupParams.bExecuteCommandLine)
		GetISystem()->ExecuteCommandLine();

	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

#if defined(CRY_UNIT_TESTING)
	if (CryUnitTest::IUnitTestManager* pTestManager = GetISystem()->GetITestSystem()->GetIUnitTestManager())
	{
	#if defined(_LIB)
		pTestManager->CreateTests(CryUnitTest::Test::m_pFirst, "StaticBinary");
	#endif

		const ICmdLineArg* pSkipUnitTest = GetISystem()->GetICmdLine()->FindArg(eCLAT_Pre, "skip_unit_tests");
		if (!pSkipUnitTest)
		{
			const ICmdLineArg* pUseUnitTestExcelReporter = GetISystem()->GetICmdLine()->FindArg(eCLAT_Pre, "use_unit_test_excel_reporter");
			if (pUseUnitTestExcelReporter)
			{
				GetISystem()->GetITestSystem()->GetIUnitTestManager()->RunAllTests(CryUnitTest::ExcelReporter);
			}
			else
			{
				GetISystem()->GetITestSystem()->GetIUnitTestManager()->RunAllTests(CryUnitTest::MinimalReporter);
			}
		}
	}
#endif

	return pOut;
}

IGameRef CGameStartup::Reset()
{
	static char pGameBuffer[sizeof(CGame)];
	m_pGame = new((void*)pGameBuffer)CGame();

	if (m_pGame->Init(m_pFramework))
	{
		return m_gameRef;
	}

	return nullptr;
}

int CGameStartup::Update(bool haveFocus, unsigned int updateFlags)
{
	// The frame profile system already creates an "overhead" profile label
	// in StartFrame(). Hence we have to set the FRAMESTART before.
	CRY_PROFILE_FRAMESTART("Main");

#if defined(JOBMANAGER_SUPPORT_PROFILING)
	gEnv->GetJobManager()->SetFrameStartTime(gEnv->pTimer->GetAsyncTime());
#endif

	int returnCode = 0;

	if (m_pGame)
	{
		returnCode = m_pGame->Update(haveFocus, updateFlags);
	}

	if (!m_fullScreenCVarSetup && gEnv->pConsole)
	{
		ICVar* pVar = gEnv->pConsole->GetCVar("r_Fullscreen");
		if (pVar)
		{
			pVar->SetOnChangeCallback(FullScreenCVarChanged);
			m_fullScreenCVarSetup = true;
		}
	}

#if ENABLE_AUTO_TESTER
	s_autoTesterSingleton.Update();
#endif

	return returnCode;
}

void CGameStartup::FullScreenCVarChanged(ICVar* pVar)
{
	if (GetISystem()->GetISystemEventDispatcher())
	{
		GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_TOGGLE_FULLSCREEN, pVar->GetIVal(), 0);
	}
}

bool CGameStartup::GetRestartLevel(char** levelName)
{
	if (GetISystem()->IsRelaunch())
		*levelName = (char*)(gEnv->pGame->GetIGameFramework()->GetLevelName());
	return GetISystem()->IsRelaunch();
}

int CGameStartup::Run(const char* autoStartLevelName)
{
	gEnv->pConsole->ExecuteString("exec autoexec.cfg");

	if (autoStartLevelName)
	{
		//load savegame
		if (CryStringUtils::stristr(autoStartLevelName, CRY_SAVEGAME_FILE_EXT) != 0)
		{
			CryFixedStringT<256> fileName(autoStartLevelName);
			// NOTE! two step trimming is intended!
			fileName.Trim(" ");  // first:  remove enclosing spaces (outside ")
			fileName.Trim("\""); // second: remove potential enclosing "
			gEnv->pGame->GetIGameFramework()->LoadGame(fileName.c_str());
		}
		else  //start specified level
		{
			CryFixedStringT<256> mapCmd("map ");
			mapCmd += autoStartLevelName;
			gEnv->pConsole->ExecuteString(mapCmd.c_str());
		}
	}
	else if(!gEnv->IsDedicated())
	{
		gEnv->pConsole->ExecuteString("map example s");
	}

#if CRY_PLATFORM_WINDOWS
	if (!(gEnv && gEnv->pSystem) || (!gEnv->IsEditor() && !gEnv->IsDedicated()))
	{
		::ShowCursor(FALSE);
		if (GetISystem()->GetIHardwareMouse())
			GetISystem()->GetIHardwareMouse()->DecrementCounter();
	}
#else
	if (gEnv && gEnv->pHardwareMouse)
		gEnv->pHardwareMouse->DecrementCounter();
#endif

#if !CRY_PLATFORM_DURANGO
	for(;;)
	{
		if (!Update(true, 0))
		{
			break;
		}
	}
#endif

	return 0;
}

void CGameStartup::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_RANDOM_SEED:
		cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
		break;
	case ESYSTEM_EVENT_FAST_SHUTDOWN:
		m_quit = true;
		break;
	}
}

#if defined(_LIB)
extern "C" IGameFramework * CreateGameFramework();
#else
static HMODULE s_frameworkDLL = nullptr;

static void CleanupFrameworkDLL()
{
	assert(s_frameworkDLL);
	CryFreeLibrary(s_frameworkDLL);
	s_frameworkDLL = 0;
}
#endif

bool CGameStartup::InitFramework(SSystemInitParams& startupParams)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Init Game Framework");

#if !defined(_LIB)
	if (startupParams.szBinariesDir && startupParams.szBinariesDir[0])
	{
		string dllName = PathUtil::Make(startupParams.szBinariesDir, GAME_FRAMEWORK_FILENAME);
		s_frameworkDLL = CryLoadLibrary(dllName.c_str());
	}
	else
	{
		s_frameworkDLL = CryLoadLibrary(GAME_FRAMEWORK_FILENAME);
	}

	atexit(CleanupFrameworkDLL);

	if (!s_frameworkDLL)
	{
		CryFatalError("Failed to open the GameFramework DLL!");
		return false;
	}

	IGameFramework::TEntryFunction CreateGameFramework = (IGameFramework::TEntryFunction)CryGetProcAddress(s_frameworkDLL, "CreateGameFramework");

	if (!CreateGameFramework)
	{
		CryFatalError("Specified GameFramework DLL is not valid!");
		return false;
	}
#endif

	m_pFramework = CreateGameFramework();

	if (!m_pFramework)
	{
		CryFatalError("Failed to create the GameFramework Interface!");
		return false;
	}

	if (!m_pFramework->Init(startupParams))
	{
		CryFatalError("Failed to initialize CryENGINE!");
		return false;
	}
	
	ICVar *pGameName = m_pFramework->GetISystem()->GetIConsole()->GetCVar("sys_game_name");

	ModuleInitISystem(m_pFramework->GetISystem(), pGameName->GetString());

#if CRY_PLATFORM_WINDOWS
	if(gEnv->pRenderer != nullptr)
	{
		SetWindowLongPtr(reinterpret_cast<HWND>(gEnv->pRenderer->GetHWND()), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	}
#endif

	return true;
}

void CGameStartup::ShutdownFramework()
{
	if (m_pFramework)
	{
		m_pFramework->Shutdown();
		m_pFramework = nullptr;
	}
}