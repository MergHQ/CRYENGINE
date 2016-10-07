// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGameFramework.h>
#include <CryCore/Platform/CryWindows.h>

class CGameStartup : public IGameStartup
{
public:
	// IGameStartup
	virtual IGameRef    Init(SSystemInitParams& startupParams) override;
	virtual void        Shutdown() override;
	// ~IGameStartup

	static CGameStartup* Create();

protected:
	CGameStartup();
	virtual ~CGameStartup();

private:
	IGame*                   m_pGame;
	IGameRef                 m_gameRef;
};
