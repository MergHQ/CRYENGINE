// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryScriptSystem/ScriptHelpers.h>

struct IFunctionHandler;

// These numerical values are deprecated; use the strings instead
enum EAIPathType
{
	AIPATH_DEFAULT,
	AIPATH_HUMAN,
	AIPATH_HUMAN_COVER,
	AIPATH_CAR,
	AIPATH_TANK,
	AIPATH_BOAT,
	AIPATH_HELI,
	AIPATH_3D,
	AIPATH_SCOUT,
	AIPATH_TROOPER,
	AIPATH_HUNTER,
};

class CScriptBind_AI : public CScriptableBase
{
public:

	CScriptBind_AI(ISystem* pSystem);

	// Assign AgentPathfindingProperties to the given path type
	//
	int  AssignPFPropertiesToPathType(IFunctionHandler* pH);
	void AssignPFPropertiesToPathType(const string& sPathType, AgentPathfindingProperties& properties);

private:
	static const char* GetPathTypeName(EAIPathType pathType);
};
