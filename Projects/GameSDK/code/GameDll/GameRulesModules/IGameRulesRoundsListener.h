// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class IGameRulesRoundsListener
{
public:
	virtual ~IGameRulesRoundsListener() {}

	virtual void OnRoundStart() = 0;
	virtual void OnRoundEnd() = 0;
	virtual void OnSuddenDeath() = 0;
	virtual void ClRoundsNetSerializeReadState(int newState, int curState) = 0;
	virtual void OnRoundAboutToStart() = 0;

};
