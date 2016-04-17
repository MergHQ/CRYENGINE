// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#pragma once

#include <CryGame/IGameFramework.h>
#include <CryCore/Platform/CryWindows.h>

#define GAME_FRAMEWORK_FILENAME CryLibraryDefName("CryAction")
#define GAME_WINDOW_CLASSNAME "CRYENGINE"

extern HMODULE GetFrameworkDLL(const char* dllLocalDir);

class GameStartupErrorObserver : public IErrorObserver
{
	public:
		// IErrorObserver
		void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber) override;
		void OnFatalError(const char* message) override;
		// ~IErrorObserver
};


class CGameStartup : public IGameStartup, public ISystemEventListener
{
public:
	// IGameStartup
	virtual IGameRef Init(SSystemInitParams& startupParams) override;
	virtual void Shutdown() override;
	virtual int Update(bool haveFocus, unsigned int updateFlags) override;
	virtual bool GetRestartLevel(char** levelName) override;
	virtual const char* GetPatch() const override { return nullptr; }
	virtual bool GetRestartMod(char* pModName, int nameLenMax) override { return false; }
	virtual int Run(const char* autoStartLevelName) override;
	// ~IGameStartup

	virtual IGameRef Reset();

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	bool InitFramework(SSystemInitParams& startupParams);

	static CGameStartup* Create();

protected:
	CGameStartup();
	virtual ~CGameStartup();

private:
	void ShutdownFramework();

	static void FullScreenCVarChanged(ICVar* pVar);
		
#if CRY_PLATFORM_WINDOWS
	// IWindowMessageHandler
	bool HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	// ~IWindowMessageHandler
#endif

	IGame* m_pGame;
	IGameRef m_gameRef;
	bool m_quit;
	bool m_fullScreenCVarSetup;

	HMODULE	m_gameDll;
	HMODULE m_frameworkDll;

	string m_reqModName;
	
	IGameFramework* m_pFramework;
	GameStartupErrorObserver m_errorObsever;
};
