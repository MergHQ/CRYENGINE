// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __GAMESTARTUP_H__
#define __GAMESTARTUP_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <CryGame/IGameFramework.h>
#include <CrySystem/IWindowMessageHandler.h>

#define GAME_FRAMEWORK_FILENAME CryLibraryDefName("CryAction")

// implemented in GameDll.cpp
extern HMODULE GetFrameworkDLL(const char* dllLocalDir);

class CGameStartupStatic;

#if defined(CVARS_WHITELIST)
class CCVarsWhiteList : public ICVarsWhitelist
{
public:
	// ICVarsWhiteList
	bool IsWhiteListed(const string& command, bool silent);
	// ~ICVarsWhiteList

protected:
private:
};
#endif // defined(CVARS_WHITELIST)

class GameStartupErrorObserver : public IErrorObserver
{
	public:
		void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber);
		void OnFatalError(const char* message);
};

class CGameStartup :
	public IGameStartup, public ISystemEventListener, public IWindowMessageHandler
{
	friend class CGameStartupStatic; // to have access to m_pFramework and m_reqModName in RequestLoadMod
public:
	virtual IGameRef Init(SSystemInitParams &startupParams);
	virtual IGameRef Reset(const char *modName);
	virtual void Shutdown();
	virtual int Update(bool haveFocus, unsigned int updateFlags);
	virtual bool GetRestartLevel(char** levelName);
	virtual const char* GetPatch() const;
	virtual bool GetRestartMod(char* pModNameBuffer, int modNameBufferSizeInBytes);
	virtual int Run( const char * autoStartLevelName );
	virtual const uint8* GetRSAKey(uint32 *pKeySize) const;

	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

	static CGameStartup* Create();

protected:
	CGameStartup();
	virtual ~CGameStartup();

private:
	bool IsModAvailable(const string& modName);
	void HandleResizeForVOIP(WPARAM wparam);

public:

	bool InitFramework(SSystemInitParams &startupParams);
private:
	void ShutdownFramework();

	static void FullScreenCVarChanged( ICVar *pVar );
	
#if CRY_PLATFORM_WINDOWS
	bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
#endif

	IGame*   m_pMod;
	IGameRef m_modRef;
	bool     m_quit;
	bool     m_fullScreenCVarSetup;
	int8     m_nVOIPWasActive;
	HMODULE  m_modDll;
	HMODULE  m_frameworkDll;

	string   m_reqModName;
	bool     m_reqModUnload;

	IGameFramework* m_pFramework;
	GameStartupErrorObserver m_errorObsever;
};

class CGameStartupStatic
{
	friend class CGameStartup; // to set and unset g_pGameStartup on construction and destruction of CGameStartup
public:
	static void RequestLoadMod(IConsoleCmdArgs* pCmdArgs);
	static void RequestUnloadMod(IConsoleCmdArgs* pCmdArgs);
	static void ForceCursorUpdate();
private:
	static CGameStartup* g_pGameStartup;
};

#endif //__GAMESTARTUP_H__
