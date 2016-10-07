// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description:

   -------------------------------------------------------------------------
   History:
   - 15:9:2004   10:30 : Created by Mathieu Pinard
   - 20:10:2004   10:30 : Added IGameRulesSystem interface (Craig Tiller)

*************************************************************************/
#ifndef __GAMERULESYSTEM_H__
#define __GAMERULESYSTEM_H__

#if _MSC_VER > 1000
	#pragma once
#endif

#include "IGameRulesSystem.h"

class CGameServerNub;

class CGameRulesSystem : public IGameRulesSystem
{
	typedef struct SGameRulesDef
	{
		string              extension;
		std::vector<string> aliases;
		std::vector<string> maplocs;

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			pSizer->AddObject(extension);
			pSizer->AddObject(aliases);
			pSizer->AddObject(maplocs);
		}
	}SGameRulesDef;

	typedef std::map<string, SGameRulesDef> TGameRulesMap;
public:
	CGameRulesSystem(ISystem* pSystem, IGameFramework* pGameFW);
	~CGameRulesSystem();

	void                Release() { delete this; };

	virtual bool        RegisterGameRules(const char* rulesName, const char* extensionName, bool bUseScript);
	virtual bool        CreateGameRules(const char* rulesName);
	virtual bool        DestroyGameRules();
	virtual bool        HaveGameRules(const char* rulesName);

	virtual void        AddGameRulesAlias(const char* gamerules, const char* alias);
	virtual void        AddGameRulesLevelLocation(const char* gamerules, const char* mapLocation);
	virtual const char* GetGameRulesLevelLocation(const char* gamerules, int i);

	virtual void        SetCurrentGameRules(IGameRules* pGameRules);
	virtual IGameRules* GetCurrentGameRules() const;
	const char*         GetGameRulesName(const char* alias) const;

	void                GetMemoryStatistics(ICrySizer* s);

private:
	SGameRulesDef*         GetGameRulesDef(const char* name);
	static IEntityProxyPtr CreateGameObject(
	  IEntity* pEntity, SEntitySpawnParams& params, void* pUserData);

	IGameFramework* m_pGameFW;

	EntityId        m_currentGameRules;
	IGameRules*     m_pGameRules;

	TGameRulesMap   m_GameRules;
};

#endif // __GAMERULESYSTEM_H__
