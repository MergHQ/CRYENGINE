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
				pData = gEnv->pAISystem->GetSignalManager()->CreateSignalExtraData();
				CRY_ASSERT(pData);
				pData->FromScriptTable(dataTable);
			}
		}

		CRangeSignaling::ref().AddRangeSignal((EntityId)entityId.n, fRadius, fFlexibleBoundary, sSignal, pData);
		gEnv->pAISystem->GetSignalManager()->FreeSignalExtraData(pData);
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
				pData = gEnv->pAISystem->GetSignalManager()->CreateSignalExtraData();
				CRY_ASSERT(pData);
				pData->FromScriptTable(dataTable);
			}
		}

		CRangeSignaling::ref().AddTargetRangeSignal((EntityId)entityId.n, (EntityId)targetId.n, fRadius, fFlexibleBoundary, sSignal, pData);
		gEnv->pAISystem->GetSignalManager()->FreeSignalExtraData(pData);
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
				pData = gEnv->pAISystem->GetSignalManager()->CreateSignalExtraData();
				CRY_ASSERT(pData);
				pData->FromScriptTable(dataTable);
			}
		}

		CRangeSignaling::ref().AddAngleSignal((EntityId)entityId.n, fAngle, fFlexibleBoundary, sSignal, pData);
		gEnv->pAISystem->GetSignalManager()->FreeSignalExtraData(pData);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_AIAction::SetAimQueryMode(IFunctionHandler* pH, ScriptHandle entityId, int mode)
{
	IEntity* entity = gEnv->pEntitySystem->GetEntity(static_cast<EntityId>(entityId.n));
	IAIObject* ai = entity ? GetEntityAIObject(entity) : NULL;
	CAIProxy* proxy = ai ? static_cast<CAIProxy*>(ai->GetProxy()) : NULL;

	if (proxy)
		proxy->SetAimQueryMode(static_cast<CAIProxy::AimQueryMode>(mode));

	return pH->EndFunction();
}
