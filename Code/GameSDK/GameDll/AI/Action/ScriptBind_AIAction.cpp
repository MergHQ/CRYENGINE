// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ScriptBind_AIAction.h"

#include "AI/Action/AIProxy.h"
#include "AI/Action/SignalTimers/SignalTimers.h"
#include "AI/Action/RangeSignalingSystem/RangeSignaling.h"

#include <IVehicleSystem.h>
#include <IActorSystem.h>

#include <CryAISystem/IAIObjectManager.h>

//------------------------------------------------------------------------
CScriptBind_AIAction::CScriptBind_AIAction(CGameAISystem* pGameAISystem)
	: m_pGameAISystem(pGameAISystem)
{
	Init(gEnv->pScriptSystem, GetISystem());

	SetGlobalName("CryAction");

	gEnv->pScriptSystem->SetGlobalValue("QueryAimFromMovementController", CAIProxy::QueryAimFromMovementController);
	gEnv->pScriptSystem->SetGlobalValue("OverriddenAndAiming", CAIProxy::OverriddenAndAiming);
	gEnv->pScriptSystem->SetGlobalValue("OverriddenAndNotAiming", CAIProxy::OverriddenAndNotAiming);

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_AIAction::

	SCRIPT_REG_TEMPLFUNC(EnableSignalTimer, "entityId, text");
	SCRIPT_REG_TEMPLFUNC(DisableSignalTimer, "entityId, text");
	SCRIPT_REG_TEMPLFUNC(SetSignalTimerRate, "entityId, text, float, float");
	SCRIPT_REG_TEMPLFUNC(ResetSignalTimer, "entityId, text");

	SCRIPT_REG_TEMPLFUNC(EnableRangeSignaling, "entityId, bEnable");
	SCRIPT_REG_TEMPLFUNC(DestroyRangeSignaling, "entityId");
	SCRIPT_REG_TEMPLFUNC(ResetRangeSignaling, "entityId");

	SCRIPT_REG_TEMPLFUNC(AddRangeSignal, "entityId, float, float, text");
	SCRIPT_REG_TEMPLFUNC(AddTargetRangeSignal, "entityId, targetId, float, float, text");
	SCRIPT_REG_TEMPLFUNC(AddAngleSignal, "entityId, float, float, text");

	SCRIPT_REG_TEMPLFUNC(SetAimQueryMode, "entityId, mode");

	//SCRIPT_REG_FUNC(RegisterWithAI);
}

//-------------------------------------------------------------------------
int CScriptBind_AIAction::EnableSignalTimer(IFunctionHandler* pH, ScriptHandle entityId, const char* sText)
{
	bool bRet = CSignalTimer::ref().EnablePersonalManager((EntityId)entityId.n, sText);
	return pH->EndFunction(bRet);
}

//-------------------------------------------------------------------------
int CScriptBind_AIAction::DisableSignalTimer(IFunctionHandler* pH, ScriptHandle entityId, const char* sText)
{
	bool bRet = CSignalTimer::ref().DisablePersonalSignalTimer((EntityId)entityId.n, sText);
	return pH->EndFunction(bRet);
}

//-------------------------------------------------------------------------
int CScriptBind_AIAction::SetSignalTimerRate(IFunctionHandler* pH, ScriptHandle entityId, const char* sText, float fRateMin, float fRateMax)
{
	bool bRet = CSignalTimer::ref().SetTurnRate((EntityId)entityId.n, sText, fRateMin, fRateMax);
	return pH->EndFunction(bRet);
}

//-------------------------------------------------------------------------
int CScriptBind_AIAction::ResetSignalTimer(IFunctionHandler* pH, ScriptHandle entityId, const char* sText)
{
	bool bRet = CSignalTimer::ref().ResetPersonalTimer((EntityId)entityId.n, sText);
	return pH->EndFunction(bRet);
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::EnableRangeSignaling(IFunctionHandler* pH, ScriptHandle entityId, bool bEnable)
{
	CRangeSignaling::ref().EnablePersonalRangeSignaling((EntityId)entityId.n, bEnable);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::DestroyRangeSignaling(IFunctionHandler* pH, ScriptHandle entityId)
{
	CRangeSignaling::ref().DestroyPersonalRangeSignaling((EntityId)entityId.n);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::ResetRangeSignaling(IFunctionHandler* pH, ScriptHandle entityId)
{
	CRangeSignaling::ref().ResetPersonalRangeSignaling((EntityId)entityId.n);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::AddRangeSignal(IFunctionHandler* pH, ScriptHandle entityId, float fRadius, float fFlexibleBoundary, const char* sSignal)
{
	if (gEnv->pAISystem)
	{
		// Get optional signal data
		AISignals::IAISignalExtraData* pData = NULL;
		if (pH->GetParamCount() > 4)
		{
			SmartScriptTable dataTable;
			if (pH->GetParam(5, dataTable))
			{
				pData = gEnv->pAISystem->CreateSignalExtraData();
				CRY_ASSERT(pData);
				pData->FromScriptTable(dataTable);
			}
		}

		CRangeSignaling::ref().AddRangeSignal((EntityId)entityId.n, fRadius, fFlexibleBoundary, sSignal, pData);
		gEnv->pAISystem->FreeSignalExtraData(pData);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::AddTargetRangeSignal(IFunctionHandler* pH, ScriptHandle entityId, ScriptHandle targetId, float fRadius, float fFlexibleBoundary, const char* sSignal)
{
	if (gEnv->pAISystem)
	{
		// Get optional signal data
		AISignals::IAISignalExtraData* pData = NULL;
		if (pH->GetParamCount() > 5)
		{
			SmartScriptTable dataTable;
			if (pH->GetParam(6, dataTable))
			{
				pData = gEnv->pAISystem->CreateSignalExtraData();
				CRY_ASSERT(pData);
				pData->FromScriptTable(dataTable);
			}
		}

		CRangeSignaling::ref().AddTargetRangeSignal((EntityId)entityId.n, (EntityId)targetId.n, fRadius, fFlexibleBoundary, sSignal, pData);
		gEnv->pAISystem->FreeSignalExtraData(pData);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::AddAngleSignal(IFunctionHandler* pH, ScriptHandle entityId, float fAngle, float fFlexibleBoundary, const char* sSignal)
{
	if (gEnv->pAISystem)
	{
		// Get optional signal data
		AISignals::IAISignalExtraData* pData = NULL;
		if (pH->GetParamCount() > 4)
		{
			SmartScriptTable dataTable;
			if (pH->GetParam(5, dataTable))
			{
				pData = gEnv->pAISystem->CreateSignalExtraData();
				CRY_ASSERT(pData);
				pData->FromScriptTable(dataTable);
			}
		}

		CRangeSignaling::ref().AddAngleSignal((EntityId)entityId.n, fAngle, fFlexibleBoundary, sSignal, pData);
		gEnv->pAISystem->FreeSignalExtraData(pData);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::SetAimQueryMode(IFunctionHandler* pH, ScriptHandle entityId, int mode)
{
	IEntity* entity = gEnv->pEntitySystem->GetEntity(static_cast<EntityId>(entityId.n));
	IAIObject* ai = entity ? entity->GetAI() : NULL;
	CAIProxy* proxy = ai ? static_cast<CAIProxy*>(ai->GetProxy()) : NULL;

	if (proxy)
		proxy->SetAimQueryMode(static_cast<CAIProxy::AimQueryMode>(mode));

	return pH->EndFunction();
}

//
//-----------------------------------------------------------------------------------------------------------
int CScriptBind_AIAction::RegisterWithAI(IFunctionHandler* pH)
{
	if (gEnv->bMultiplayer && !gEnv->bServer)
		return pH->EndFunction();

	int type;
	ScriptHandle hdl;

	if (!pH->GetParams(hdl, type))
		return pH->EndFunction();

	EntityId entityID = (EntityId)hdl.n;
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityID);

	if (!pEntity)
	{
		GameWarning("RegisterWithAI: Tried to set register with AI nonExisting entity with id [%d]. ", entityID);
		return pH->EndFunction();
	}

	// Apparently we can't assume that there is just one IGameObject to an entity, because we choose between (at least) Actor and Vehicle objects.
	// (MATT) Do we really need to check on the actor system here? {2008/02/15:18:38:34}
	IGameFramework* pGameFramework = gEnv->pGameFramework;
	IVehicleSystem* pVSystem = pGameFramework->GetIVehicleSystem();
	IActorSystem* pASystem = pGameFramework->GetIActorSystem();
	if (!pASystem)
	{
		GameWarning("RegisterWithAI: no ActorSystem for %s.", pEntity->GetName());
		return pH->EndFunction();
	}

	AIObjectParams params(type, 0, entityID);
	bool autoDisable(true);

	// For most types, we need to parse the tables
	// For others we leave them blank
	switch (type)
	{
	case AIOBJECT_ACTOR:
	case AIOBJECT_2D_FLY:
	case AIOBJECT_BOAT:
	case AIOBJECT_CAR:
	case AIOBJECT_HELICOPTER:

	case AIOBJECT_INFECTED:
	case AIOBJECT_ALIENTICK:

	case AIOBJECT_HELICOPTERCRYSIS2:
		if (gEnv->pAISystem && !gEnv->pAISystem->ParseTables(3, true, pH, params, autoDisable))
			return pH->EndFunction();
	default:
		;
	}

	// Most types check these, so just get them in advance
	IActor* pActor = pASystem->GetActor(pEntity->GetId());

	IVehicle* pVehicle = NULL;
	if (pVSystem)
		pVehicle = pVSystem->GetVehicle(pEntity->GetId());

	// Set this if we've found something to create a proxy from
	IGameObject* pGameObject = NULL;

	switch (type)
	{
	case AIOBJECT_ACTOR:
	case AIOBJECT_2D_FLY:

	case AIOBJECT_INFECTED:
	case AIOBJECT_ALIENTICK:
	{
		// (MATT) The pActor/pVehicle test below - is it basically trying to distiguish between the two cases above? If so, separate them! {2008/02/15:19:38:08}
		if (!pActor)
		{
			GameWarning("RegisterWithAI: no Actor for %s.", pEntity->GetName());
			return pH->EndFunction();
		}
		pGameObject = pActor->GetGameObject();
	}
	break;
	case AIOBJECT_BOAT:
	case AIOBJECT_CAR:
	{
		if (!pVehicle)
		{
			GameWarning("RegisterWithAI: no Vehicle for %s (Id %i).", pEntity->GetName(), pEntity->GetId());
			return pH->EndFunction();
		}
		pGameObject = pVehicle->GetGameObject();
	}
	break;

	case AIOBJECT_HELICOPTER:
	case AIOBJECT_HELICOPTERCRYSIS2:
	{
		if (!pVehicle)
		{
			GameWarning("RegisterWithAI: no Vehicle for %s (Id %i).", pEntity->GetName(), pEntity->GetId());
			return pH->EndFunction();
		}
		pGameObject = pVehicle->GetGameObject();
		params.m_moveAbility.b3DMove = true;
	}
	break;
	case AIOBJECT_PLAYER:
	{
		if (IsDemoPlayback())
			return pH->EndFunction();

		SmartScriptTable pTable;

		if (pH->GetParamCount() > 2)
			pH->GetParam(3, pTable);
		else
			return pH->EndFunction();

		pGameObject = pActor->GetGameObject();

		pTable->GetValue("groupid", params.m_sParamStruct.m_nGroup);

		const char* faction = 0;
		if (pTable->GetValue("esFaction", faction) && gEnv->pAISystem)
		{
			params.m_sParamStruct.factionID = gEnv->pAISystem->GetFactionMap().GetFactionID(faction);
			if (faction && *faction && (params.m_sParamStruct.factionID == IFactionMap::InvalidFactionID))
			{
				GameWarning("Unknown faction '%s' being set...", faction);
			}
		}
		else
		{
			// Márcio: backwards compatibility
			int species = -1;
			if (!pTable->GetValue("eiSpecies", species))
				pTable->GetValue("species", species);

			if (species > -1)
				params.m_sParamStruct.factionID = species;
		}

		pTable->GetValue("commrange", params.m_sParamStruct.m_fCommRange); //Luciano - added to use GROUPONLY signals

		SmartScriptTable pPerceptionTable;
		if (pTable->GetValue("Perception", pPerceptionTable))
		{
			pPerceptionTable->GetValue("sightrange", params.m_sParamStruct.m_PerceptionParams.sightRange);
		}
	}
	break;
	case AIOBJECT_SNDSUPRESSOR:
	{
		// (MATT) This doesn't need a proxy? {2008/02/15:19:45:58}
		SmartScriptTable pTable;
		// Properties table
		if (pH->GetParamCount() > 2)
			pH->GetParam(3, pTable);
		else
			return pH->EndFunction();
		if (!pTable->GetValue("radius", params.m_moveAbility.pathRadius))
			params.m_moveAbility.pathRadius = 10.f;
		break;
	}
	case AIOBJECT_WAYPOINT:
		break;
		/*
			// this block is commented out since params.m_sParamStruct is currently ignored in pEntity->RegisterInAISystem()
			// instead of setting the group id here, it will be set from the script right after registering
	default:
		// try to get groupid settings for anchors
		params.m_sParamStruct.m_nGroup = -1;
		params.m_sParamStruct.m_nSpecies = -1;
		{
			SmartScriptTable pTable;
			if (pH->GetParamCount() > 2)
				pH->GetParam(3, pTable);
			if (*pTable)
				pTable->GetValue("groupid", params.m_sParamStruct.m_nGroup);
		}
		break;
		*/
	}

	params.name = pEntity->GetName();

	// Register in AI to get a new AI object, unregistering the old one in the process
	gEnv->pAISystem->GetAIObjectManager()->CreateAIObject(params);

	// (MATT) ? {2008/02/15:19:46:29}
	// AI object was not created (possibly AI System is disabled)
	if (IAIObject* aiObject = pEntity->GetAI())
	{
		if (type == AIOBJECT_SNDSUPRESSOR)
			aiObject->SetRadius(params.m_moveAbility.pathRadius);
		else if (type >= AIANCHOR_FIRST) // if anchor - set radius
		{
			SmartScriptTable pTable;
			// Properties table
			if (pH->GetParamCount() > 2)
				pH->GetParam(3, pTable);
			else
				return pH->EndFunction();
			float radius(0.f);
			pTable->GetValue("radius", radius);
			int groupId = -1;
			pTable->GetValue("groupid", groupId);
			aiObject->SetGroupId(groupId);
			aiObject->SetRadius(radius);
		}

		if (IAIActorProxy* proxy = aiObject->GetProxy())
			proxy->UpdateMeAlways(!autoDisable);
	}

	return pH->EndFunction();
}