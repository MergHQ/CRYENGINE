// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryScriptSystem/IScriptSystem.h>

struct IGameFramework;

class CScriptBind_HUD :
	public CScriptableBase
{
public:
	CScriptBind_HUD(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_HUD();

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

protected:
	int SetObjectiveStatus(IFunctionHandler *pH,const char* pObjectiveID, int status, bool silent);
	int SetObjectiveEntity(IFunctionHandler *pH,const char* pObjectiveID,const char* entityName);
	int ClearObjectiveEntity(IFunctionHandler *pH,const char* pObjectiveID);
	int AddEntityToRadar(IFunctionHandler *pH, ScriptHandle entityId);
	
	int RemoveEntityFromRadar(IFunctionHandler *pH, ScriptHandle entityId);
	int AddEntitySilhouette(IFunctionHandler *pH, ScriptHandle entityId, float r, float g, float b, float a);
	
	int OnGameStatusUpdate(IFunctionHandler *pH, int goodBad, const char *msg);
	int RemoveObjective(IFunctionHandler *pH, ScriptHandle entityId);


private:
	void RegisterGlobals();
	void RegisterMethods();

	ISystem						*m_pSystem;
	IGameFramework		*m_pGameFW;
};
