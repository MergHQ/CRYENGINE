// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GameCodeCoverage/GameCodeCoverageEnabled.h"
#include "GameMechanismManager/GameMechanismBase.h"

#if ENABLE_GAME_CODE_COVERAGE

class CGameCodeCoverageGUI : public CGameMechanismBase
{
public:		// Construction & destruction
	CGameCodeCoverageGUI(void);
	~CGameCodeCoverageGUI(void);

	static ILINE CGameCodeCoverageGUI * GetInstance()
	{
		return s_instance;
	}

public:		// Operations
	void Draw();

private:	// Member data
	static CGameCodeCoverageGUI * s_instance;

	virtual void Update(float dt) {}

	int m_showListWhenNumUnhitCheckpointsIs;
};

#endif	// ENABLE_GAME_CODE_COVERAGE
