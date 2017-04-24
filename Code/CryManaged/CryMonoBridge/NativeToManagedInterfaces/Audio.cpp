// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Audio.h"

#include "MonoRuntime.h"

#include "Wrappers/MonoDomain.h"
#include "Wrappers/MonoClass.h"

#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>

#include <CryAudio/IAudioSystem.h>
#include <CryAudio/IObject.h>

using namespace CryAudio;

typedef void(*AudioRequestListener)(const SRequestInfo * const);

static void LoadTrigger(uint triggerId)
{
	gEnv->pAudioSystem->LoadTrigger(triggerId);
}

static void UnloadTrigger(uint triggerId)
{
	gEnv->pAudioSystem->UnloadTrigger(triggerId);
}

static void ExecuteTrigger(uint triggerId, bool bExecuteSync)
{
	if (bExecuteSync)
	{
		SRequestUserData const data(ERequestFlags::ExecuteBlocking | ERequestFlags::CallbackOnExternalOrCallingThread| ERequestFlags::DoneCallbackOnExternalThread);
		gEnv->pAudioSystem->ExecuteTrigger(triggerId, data);
	}
	else
	{
		SRequestUserData const data(ERequestFlags::CallbackOnExternalOrCallingThread | ERequestFlags::DoneCallbackOnExternalThread);
		gEnv->pAudioSystem->ExecuteTrigger(triggerId, data);
	}
}

static void StopTrigger(uint triggerId, bool bExecuteSync)
{
	if (bExecuteSync)
	{
		SRequestUserData const data(ERequestFlags::ExecuteBlocking);
		gEnv->pAudioSystem->StopTrigger(triggerId, data);
	}
	else
	{
		gEnv->pAudioSystem->StopTrigger(triggerId);
	}
}

static uint GetAudioTriggerId(MonoString* pAudioTriggerName)
{
	const char* szTriggerName = mono_string_to_utf8(pAudioTriggerName);
	ControlId triggerId = InvalidControlId;
	gEnv->pAudioSystem->GetAudioTriggerId(szTriggerName, triggerId);
	return triggerId;
}

static uint GetAudioParameterId(MonoString* pAudioParameterName)
{
	const char* szAudioParameterName = mono_string_to_utf8(pAudioParameterName);
	ControlId parameterId = InvalidControlId;
	gEnv->pAudioSystem->GetAudioParameterId(szAudioParameterName, parameterId);
	return parameterId;
}

static void SetAudioParameter(uint parameterId, float parameterValue)
{
	gEnv->pAudioSystem->SetParameter(parameterId, parameterValue);
}

static uint GetAudioSwitchId(MonoString* pAudioSwitchName)
{
	const char* szAudioSwitchName = mono_string_to_utf8(pAudioSwitchName);
	ControlId switchId = InvalidControlId;
	gEnv->pAudioSystem->GetAudioSwitchId(szAudioSwitchName, switchId);
	return switchId;
}

static uint GetAudioSwitchStateId(uint audioSwitchId, MonoString* pAudioSwitchStateName)
{
	const char* szAudioSwitchStateName = mono_string_to_utf8(pAudioSwitchStateName);
	SwitchStateId switchStateId = InvalidSwitchStateId;
	gEnv->pAudioSystem->GetAudioSwitchStateId(audioSwitchId, szAudioSwitchStateName, switchStateId);
	return switchStateId;
}

static void SetAudioSwitchState(uint audioSwitchId, uint switchStateId)
{
	gEnv->pAudioSystem->SetSwitchState(audioSwitchId, switchStateId);
}

static void StopAllSounds()
{
	gEnv->pAudioSystem->StopAllSounds();
}

static MonoString* GetConfigPath()
{
	const char* szAudioConfigPath = gEnv->pAudioSystem->GetConfigPath();
	return mono_string_new_wrapper(szAudioConfigPath);
}

static void PlayFile(SPlayFileInfo* pPlayFileInfo)
{
	gEnv->pAudioSystem->PlayFile(*pPlayFileInfo);
}

static void StopFile(SPlayFileInfo* pPlayFileInfo)
{
	gEnv->pAudioSystem->StopFile(pPlayFileInfo->szFile);
}

static void EnableAllSound(bool bIsEnabled)
{
	if (bIsEnabled)
	{
		gEnv->pAudioSystem->UnmuteAll();
	}
	else
	{
		gEnv->pAudioSystem->MuteAll();
	}
}

// IObject interface
static IObject* CreateAudioObject()
{
	IObject* pAudioObject = gEnv->pAudioSystem->CreateObject();
	return pAudioObject;
}

static CObjectTransformation* CreateAudioTransformation(float m00, float m01, float m02, float m03,
																												float m10, float m11, float m12, float m13, 
																												float m20, float m21, float m22, float m23)
{
	Matrix34_tpl<float> m34 = Matrix34_tpl<float>(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23);
	CObjectTransformation* pAudioTransformation = new CObjectTransformation(m34);
	return pAudioTransformation;
}

static void SetAudioTransformation(IObject* pAudioObject, CObjectTransformation* pObjectTransformation)
{
	pAudioObject->SetTransformation(*pObjectTransformation);
}

static void ReleaseAudioTransformation(CObjectTransformation* pObjectTransformation)
{
	delete pObjectTransformation;
}

static void ReleaseAudioObject(IObject* pAudioObject)
{
	gEnv->pAudioSystem->ReleaseObject(pAudioObject);
}

static void ExecuteAudioObjectTrigger(IObject* pAudioObject, uint triggerId, bool bExecuteSync)
{
	if (bExecuteSync)
	{
		const SRequestUserData data(ERequestFlags::ExecuteBlocking | ERequestFlags::CallbackOnExternalOrCallingThread | ERequestFlags::DoneCallbackOnExternalThread);
		pAudioObject->ExecuteTrigger(triggerId, data);
	}
	else
	{
		const SRequestUserData data(ERequestFlags::CallbackOnExternalOrCallingThread | ERequestFlags::DoneCallbackOnExternalThread);
		pAudioObject->ExecuteTrigger(triggerId, data);
	}
	
}

static void StopAudioObjectTrigger(IObject* pAudioObject, uint triggerId, bool bExecuteSync)
{
	if (bExecuteSync)
	{
		const SRequestUserData data(ERequestFlags::ExecuteBlocking );
		pAudioObject->StopTrigger(triggerId, data);
	}
	else
	{
		pAudioObject->StopTrigger(triggerId);
	}
}

static SPlayFileInfo* CreateSPlayFileInfo(MonoString* pFilePath)
{
	const char* const szAudioFilePath = mono_string_to_utf8(pFilePath);
	SPlayFileInfo* pPlayFileInfo = new SPlayFileInfo(szAudioFilePath);

	return pPlayFileInfo;
}

static void ReleaseSPlayFileInfo(SPlayFileInfo* pPlayFileInfo)
{
	delete pPlayFileInfo;
}

static SRequestInfo* CreateSRequestInfo(uint eRequestResult, uint audioSystemEvent, uint CtrlId, IObject* pAudioObject)
{
	SRequestInfo* pRequestInfo = new SRequestInfo(static_cast<CryAudio::ERequestResult>(eRequestResult), nullptr, nullptr, nullptr, static_cast<CryAudio::ESystemEvents>(audioSystemEvent), (ControlId)CtrlId, pAudioObject, nullptr, nullptr);
	return pRequestInfo;
}

static void ReleaseSRequestInfo(SRequestInfo* pRequestInfo)
{
	delete pRequestInfo;
}

static uint SRIGetRequestResult(SRequestInfo* pRequestInfo)
{
	return static_cast<uint>(pRequestInfo->requestResult);
}

static uint SRIGetSystemEvent(SRequestInfo* pRequestInfo)
{
	return static_cast<uint>(pRequestInfo->systemEvent);
}

static uint SRIGetControlId(SRequestInfo* pRequestInfo)
{
	return static_cast<uint>(pRequestInfo->audioControlId);
}

static IObject* SRIGetAudioObject(SRequestInfo* pRequestInfo)
{
	return pRequestInfo->pAudioObject;
}

static void AddAudioRequestListener(AudioRequestListener listener)
{
	gEnv->pAudioSystem->AddRequestListener(listener, nullptr, ESystemEvents::TriggerExecuted | ESystemEvents::TriggerFinished);
}

void CAudioInterface::RegisterFunctions(std::function<void(const void* pMethod, const char* szMethodName)> func)
{
	// IAudioSystem
	func(LoadTrigger, "LoadTrigger");
	func(UnloadTrigger, "UnloadTrigger");
	func(ExecuteTrigger, "ExecuteTrigger");
	func(StopTrigger, "StopTrigger");
	func(GetAudioTriggerId, "GetAudioTriggerId");
	func(GetAudioParameterId, "GetAudioParameterId");
	func(SetAudioParameter, "SetAudioParameter");
	func(GetAudioSwitchId, "GetAudioSwitchId");
	func(GetAudioSwitchStateId, "GetAudioSwitchStateId");
	func(SetAudioSwitchState, "SetAudioSwitchState");
	func(CreateAudioObject, "CreateAudioObject");
	func(StopAllSounds, "StopAllSounds");
	func(GetConfigPath, "GetConfigPath");
	func(PlayFile, "PlayFile");
	func(StopFile, "StopFile");
	func(EnableAllSound, "EnableAllSound");
	func(AddAudioRequestListener, "AddAudioRequestListener");
	
	// IObject
	func(CreateAudioObject, "CreateAudioObject");
	func(CreateAudioTransformation, "CreateAudioTransformation");
	func(ReleaseAudioTransformation, "ReleaseAudioTransformation");
	func(SetAudioTransformation, "SetAudioTransformation");
	func(ReleaseAudioObject, "ReleaseAudioObject");
	func(ExecuteAudioObjectTrigger, "ExecuteAudioObjectTrigger");
	func(StopAudioObjectTrigger, "StopAudioObjectTrigger");

	//SPlayFileInfo
	func(CreateSPlayFileInfo, "CreateSPlayFileInfo");
	func(ReleaseSPlayFileInfo, "ReleaseSPlayFileInfo");

	//SRequestInfo
	func(CreateSRequestInfo, "CreateSRequestInfo");
	func(ReleaseSRequestInfo, "ReleaseSRequestInfo");
	func(SRIGetRequestResult, "SRIGetRequestResult");
	func(SRIGetSystemEvent, "SRIGetEnumFlagsType");
	func(SRIGetControlId, "SRIGetControlId");
	func(SRIGetAudioObject, "SRIGetAudioObject");
}