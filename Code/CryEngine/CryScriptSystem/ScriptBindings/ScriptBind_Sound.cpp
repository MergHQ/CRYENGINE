// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ScriptBind_Sound.h"
#include <CryAudio/IAudioSystem.h>

CScriptBind_Sound::CScriptBind_Sound(IScriptSystem* pScriptSystem, ISystem* pSystem)
{
	CScriptableBase::Init(pScriptSystem, pSystem);
	SetGlobalName("Sound");

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_Sound::

	// AudioSystem
	SCRIPT_REG_TEMPLFUNC(GetAudioTriggerID, "sTriggerName");
	SCRIPT_REG_TEMPLFUNC(GetAudioSwitchID, "sSwitchName");
	SCRIPT_REG_TEMPLFUNC(GetAudioSwitchStateID, "hSwitchID, sStateName");
	SCRIPT_REG_TEMPLFUNC(GetAudioRtpcID, "sRtpcName");
	SCRIPT_REG_TEMPLFUNC(GetAudioEnvironmentID, "sEnvironmentName");
	SCRIPT_REG_TEMPLFUNC(SetAudioRtpcValue, "hRtpcID, fValue");
	SCRIPT_REG_TEMPLFUNC(GetAudioTriggerRadius, "triggerId");
	SCRIPT_REG_TEMPLFUNC(GetAudioTriggerOcclusionFadeOutDistance, "triggerId");

}

//////////////////////////////////////////////////////////////////////////
CScriptBind_Sound::~CScriptBind_Sound()
{
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioTriggerID(IFunctionHandler* pH, char const* const sTriggerName)
{
	if ((sTriggerName != nullptr) && (sTriggerName[0] != '\0'))
	{
		CryAudio::ControlId nTriggerID = CryAudio::InvalidControlId;
		if (gEnv->pAudioSystem->GetAudioTriggerId(sTriggerName, nTriggerID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nTriggerID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioSwitchID(IFunctionHandler* pH, char const* const sSwitchName)
{
	if ((sSwitchName != nullptr) && (sSwitchName[0] != '\0'))
	{
		CryAudio::ControlId nSwitchID = CryAudio::InvalidControlId;
		if (gEnv->pAudioSystem->GetAudioSwitchId(sSwitchName, nSwitchID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nSwitchID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioSwitchStateID(IFunctionHandler* pH, ScriptHandle const hSwitchID, char const* const sSwitchStateName)
{
	if ((sSwitchStateName != nullptr) && (sSwitchStateName[0] != '\0'))
	{
		CryAudio::SwitchStateId nSwitchStateID = CryAudio::InvalidSwitchStateId;
		CryAudio::ControlId nSwitchID = HandleToInt<CryAudio::ControlId>(hSwitchID);
		if (gEnv->pAudioSystem->GetAudioSwitchStateId(nSwitchID, sSwitchStateName, nSwitchStateID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nSwitchStateID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioRtpcID(IFunctionHandler* pH, char const* const sRtpcName)
{
	if ((sRtpcName != nullptr) && (sRtpcName[0] != '\0'))
	{
		CryAudio::ControlId parameterId = CryAudio::InvalidControlId;
		if (gEnv->pAudioSystem->GetAudioParameterId(sRtpcName, parameterId))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(parameterId));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioEnvironmentID(IFunctionHandler* pH, char const* const sEnvironmentName)
{
	if ((sEnvironmentName != nullptr) && (sEnvironmentName[0] != '\0'))
	{
		CryAudio::EnvironmentId nEnvironmentID = CryAudio::InvalidEnvironmentId;
		if (gEnv->pAudioSystem->GetAudioEnvironmentId(sEnvironmentName, nEnvironmentID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nEnvironmentID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::SetAudioRtpcValue(IFunctionHandler* pH, ScriptHandle const hRtpcID, float const fValue)
{
	gEnv->pAudioSystem->SetParameter(HandleToInt<CryAudio::ControlId>(hRtpcID), fValue);
	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioTriggerRadius(IFunctionHandler* pH, ScriptHandle const hTriggerID)
{
	CryAudio::STriggerData data;
	gEnv->pAudioSystem->GetAudioTriggerData(HandleToInt<CryAudio::ControlId>(hTriggerID), data);
	return pH->EndFunction(data.radius);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioTriggerOcclusionFadeOutDistance(IFunctionHandler* pH, ScriptHandle const hTriggerID)
{
	CryAudio::STriggerData data;
	gEnv->pAudioSystem->GetAudioTriggerData(HandleToInt<CryAudio::ControlId>(hTriggerID), data);
	return pH->EndFunction(data.occlusionFadeOutDistance);
}
