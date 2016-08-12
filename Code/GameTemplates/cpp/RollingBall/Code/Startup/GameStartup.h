// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGameFramework.h>
#include <CryCore/Platform/CryWindows.h>

#define GAME_FRAMEWORK_FILENAME CryLibraryDefName("CryAction")

extern HMODULE GetFrameworkDLL(const char* dllLocalDir);

class CGameStartup : public IGameStartup, public ISystemEventListener
{
public:
	CGameStartup();

	// IGameStartup
	virtual IGameRef    Init(SSystemInitParams& startupParams) override;
	virtual void        Shutdown() override { this->~CGameStartup(); }
	virtual int         Update(bool haveFocus, unsigned int updateFlags) override;
	virtual bool        GetRestartLevel(char** levelName) override;
	virtual const char* GetPatch() const override                              { return nullptr; }
	virtual bool        GetRestartMod(char* pModName, int nameLenMax) override { return false; }
	virtual int         Run(const char* autoStartLevelName) override;
	// ~IGameStartup

	virtual IGameRef Reset();

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	bool                 InitFramework(SSystemInitParams& startupParams);

protected:
	virtual ~CGameStartup();

private:
	void        ShutdownFramework();

	static void FullScreenCVarChanged(ICVar* pVar);

	IGame*                   m_pGame;
	IGameRef                 m_gameRef;
	bool                     m_quit;
	bool                     m_fullScreenCVarSetup;

	HMODULE                  m_gameDll;

	string                   m_reqModName;

	IGameFramework*          m_pFramework;
};
