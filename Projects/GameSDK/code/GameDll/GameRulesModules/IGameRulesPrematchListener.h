// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if USE_PC_PREMATCH
class IGameRulesPrematchListener
{
public:
	virtual ~IGameRulesPrematchListener() {}
	virtual void OnPrematchEnd() = 0;
};
#endif //#USE_PC_PREMATCH
