// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include "Common/PoolObject.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CListener;
class CObject;

enum class ESystemRequestType : EnumFlagsType
{
	None,
	SetImpl,
	ReleaseImpl,
	RefreshSystem,
	StopAllSounds,
	ParseControlsData,
	ParsePreloadsData,
	ClearControlsData,
	ClearPreloadsData,
	PreloadSingleRequest,
	UnloadSingleRequest,
	SetParameter,
	SetGlobalParameter,
	SetSwitchState,
	SetGlobalSwitchState,
	LoadTrigger,
	UnloadTrigger,
	PlayFile,
	StopFile,
	AutoLoadSetting,
	LoadSetting,
	UnloadSetting,
	UnloadAFCMDataByScope,
	DrawDebugInfo,
	AddRequestListener,
	RemoveRequestListener,
	ChangeLanguage,
	RetriggerControls,
	ReleasePendingRays,
	ReloadControlsData,
	GetFileData,
	GetImplInfo,
	RegisterListener,
	ReleaseListener,
	RegisterObject,
	ReleaseObject,
	ExecuteTrigger,
	ExecuteTriggerEx,
	ExecuteDefaultTrigger,
	ExecutePreviewTrigger,
	ExecutePreviewTriggerEx,
	StopTrigger,
	StopAllTriggers,
	StopPreviewTrigger,
	ResetRequestCount,
};

//////////////////////////////////////////////////////////////////////////
struct SSystemRequestDataBase : public SRequestData
{
	explicit SSystemRequestDataBase(ESystemRequestType const systemRequestType_)
		: SRequestData(ERequestType::SystemRequest)
		, systemRequestType(systemRequestType_)
	{}

	virtual ~SSystemRequestDataBase() override = default;

	ESystemRequestType const systemRequestType;
};

//////////////////////////////////////////////////////////////////////////
template<ESystemRequestType T>
struct SSystemRequestData final : public SSystemRequestDataBase
{
	SSystemRequestData()
		: SSystemRequestDataBase(T)
	{}

	explicit SSystemRequestData(SSystemRequestData<T> const* const pAMRData)
		: SSystemRequestDataBase(T)
	{}

	virtual ~SSystemRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::SetImpl> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(Impl::IImpl* const pIImpl_)
		: SSystemRequestDataBase(ESystemRequestType::SetImpl)
		, pIImpl(pIImpl_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::SetImpl> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::SetImpl)
		, pIImpl(pAMRData->pIImpl)
	{}

	virtual ~SSystemRequestData() override = default;

	Impl::IImpl* const pIImpl;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::AddRequestListener> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(
		void const* const pObjectToListenTo_,
		void(*func_)(SRequestInfo const* const),
		ESystemEvents const eventMask_)
		: SSystemRequestDataBase(ESystemRequestType::AddRequestListener)
		, pObjectToListenTo(pObjectToListenTo_)
		, func(func_)
		, eventMask(eventMask_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::AddRequestListener> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::AddRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
		, eventMask(pAMRData->eventMask)
	{}

	virtual ~SSystemRequestData() override = default;

	void const* const   pObjectToListenTo;
	void                (* func)(SRequestInfo const* const);
	ESystemEvents const eventMask;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::RemoveRequestListener> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(void const* const pObjectToListenTo_, void(*func_)(SRequestInfo const* const))
		: SSystemRequestDataBase(ESystemRequestType::RemoveRequestListener)
		, pObjectToListenTo(pObjectToListenTo_)
		, func(func_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::RemoveRequestListener> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::RemoveRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
	{}

	virtual ~SSystemRequestData() override = default;

	void const* const pObjectToListenTo;
	void              (* func)(SRequestInfo const* const);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ParseControlsData> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(char const* const szFolderPath_, EDataScope const dataScope_)
		: SSystemRequestDataBase(ESystemRequestType::ParseControlsData)
		, folderPath(szFolderPath_)
		, dataScope(dataScope_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ParseControlsData> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ParseControlsData)
		, folderPath(pAMRData->folderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	EDataScope const                         dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ParsePreloadsData> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(char const* const szFolderPath_, EDataScope const dataScope_)
		: SSystemRequestDataBase(ESystemRequestType::ParsePreloadsData)
		, folderPath(szFolderPath_)
		, dataScope(dataScope_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ParsePreloadsData> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ParsePreloadsData)
		, folderPath(pAMRData->folderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	EDataScope const                         dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ClearControlsData> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(EDataScope const dataScope_)
		: SSystemRequestDataBase(ESystemRequestType::ClearControlsData)
		, dataScope(dataScope_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ClearControlsData> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ClearControlsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SSystemRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ClearPreloadsData> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(EDataScope const dataScope_)
		: SSystemRequestDataBase(ESystemRequestType::ClearPreloadsData)
		, dataScope(dataScope_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ClearPreloadsData> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ClearPreloadsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SSystemRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::PreloadSingleRequest> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(PreloadRequestId const preloadRequestId_, bool const bAutoLoadOnly_)
		: SSystemRequestDataBase(ESystemRequestType::PreloadSingleRequest)
		, preloadRequestId(preloadRequestId_)
		, bAutoLoadOnly(bAutoLoadOnly_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::PreloadSingleRequest> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::PreloadSingleRequest)
		, preloadRequestId(pAMRData->preloadRequestId)
		, bAutoLoadOnly(pAMRData->bAutoLoadOnly)
	{}

	virtual ~SSystemRequestData() override = default;

	PreloadRequestId const preloadRequestId;
	bool const             bAutoLoadOnly;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::UnloadSingleRequest> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(PreloadRequestId const preloadRequestId_)
		: SSystemRequestDataBase(ESystemRequestType::UnloadSingleRequest)
		, preloadRequestId(preloadRequestId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::UnloadSingleRequest> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::UnloadSingleRequest)
		, preloadRequestId(pAMRData->preloadRequestId)
	{}

	virtual ~SSystemRequestData() override = default;

	PreloadRequestId const preloadRequestId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::SetParameter> final : public SSystemRequestDataBase, public CPoolObject<SSystemRequestData<ESystemRequestType::SetParameter>, stl::PSyncMultiThread>
{
	explicit SSystemRequestData(ControlId const parameterId_, float const value_)
		: SSystemRequestDataBase(ESystemRequestType::SetParameter)
		, parameterId(parameterId_)
		, value(value_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::SetParameter> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::SetParameter)
		, parameterId(pASRData->parameterId)
		, value(pASRData->value)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const parameterId;
	float const     value;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::SetGlobalParameter> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(ControlId const parameterId_, float const value_)
		: SSystemRequestDataBase(ESystemRequestType::SetGlobalParameter)
		, parameterId(parameterId_)
		, value(value_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::SetGlobalParameter> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::SetGlobalParameter)
		, parameterId(pASRData->parameterId)
		, value(pASRData->value)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const parameterId;
	float const     value;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::SetSwitchState> final : public SSystemRequestDataBase, public CPoolObject<SSystemRequestData<ESystemRequestType::SetSwitchState>, stl::PSyncMultiThread>
{
	explicit SSystemRequestData(ControlId const switchId_, SwitchStateId const switchStateId_)
		: SSystemRequestDataBase(ESystemRequestType::SetSwitchState)
		, switchId(switchId_)
		, switchStateId(switchStateId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::SetSwitchState> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::SetSwitchState)
		, switchId(pASRData->switchId)
		, switchStateId(pASRData->switchStateId)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const     switchId;
	SwitchStateId const switchStateId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::SetGlobalSwitchState> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(ControlId const switchId_, SwitchStateId const switchStateId_)
		: SSystemRequestDataBase(ESystemRequestType::SetGlobalSwitchState)
		, switchId(switchId_)
		, switchStateId(switchStateId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::SetGlobalSwitchState> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::SetGlobalSwitchState)
		, switchId(pASRData->switchId)
		, switchStateId(pASRData->switchStateId)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const     switchId;
	SwitchStateId const switchStateId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::LoadTrigger> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(ControlId const triggerId_)
		: SSystemRequestDataBase(ESystemRequestType::LoadTrigger)
		, triggerId(triggerId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::LoadTrigger> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::LoadTrigger)
		, triggerId(pASRData->triggerId)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::UnloadTrigger> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(ControlId const triggerId_)
		: SSystemRequestDataBase(ESystemRequestType::UnloadTrigger)
		, triggerId(triggerId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::UnloadTrigger> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::UnloadTrigger)
		, triggerId(pASRData->triggerId)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::PlayFile> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(
		CryFixedStringT<MaxFilePathLength> const& file_,
		ControlId const usedTriggerId_,
		bool const bLocalized_)
		: SSystemRequestDataBase(ESystemRequestType::PlayFile)
		, file(file_)
		, usedTriggerId(usedTriggerId_)
		, bLocalized(bLocalized_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::PlayFile> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::PlayFile)
		, file(pASRData->file)
		, usedTriggerId(pASRData->usedTriggerId)
		, bLocalized(pASRData->bLocalized)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
	ControlId const                          usedTriggerId;
	bool const                               bLocalized;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::StopFile> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(CryFixedStringT<MaxFilePathLength> const& file_)
		: SSystemRequestDataBase(ESystemRequestType::StopFile)
		, file(file_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::StopFile> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::StopFile)
		, file(pASRData->file)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::AutoLoadSetting> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(EDataScope const scope_)
		: SSystemRequestDataBase(ESystemRequestType::AutoLoadSetting)
		, scope(scope_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::AutoLoadSetting> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::AutoLoadSetting)
		, scope(pAMRData->scope)
	{}

	virtual ~SSystemRequestData() override = default;

	EDataScope const scope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::LoadSetting> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(ControlId const id_)
		: SSystemRequestDataBase(ESystemRequestType::LoadSetting)
		, id(id_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::LoadSetting> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::LoadSetting)
		, id(pAMRData->id)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const id;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::UnloadSetting> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(ControlId const id_)
		: SSystemRequestDataBase(ESystemRequestType::UnloadSetting)
		, id(id_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::UnloadSetting> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::UnloadSetting)
		, id(pAMRData->id)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const id;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::UnloadAFCMDataByScope> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(EDataScope const dataScope_)
		: SSystemRequestDataBase(ESystemRequestType::UnloadAFCMDataByScope)
		, dataScope(dataScope_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::UnloadAFCMDataByScope> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::UnloadAFCMDataByScope)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SSystemRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::RefreshSystem> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(char const* const szLevelName)
		: SSystemRequestDataBase(ESystemRequestType::RefreshSystem)
		, levelName(szLevelName)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::RefreshSystem> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::RefreshSystem)
		, levelName(pAMRData->levelName)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxFileNameLength> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ReloadControlsData> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(char const* const szFolderPath, char const* const szLevelName)
		: SSystemRequestDataBase(ESystemRequestType::ReloadControlsData)
		, folderPath(szFolderPath)
		, levelName(szLevelName)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ReloadControlsData> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ReloadControlsData)
		, folderPath(pAMRData->folderPath)
		, levelName(pAMRData->levelName)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	CryFixedStringT<MaxFilePathLength> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::GetFileData> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(char const* const szName, SFileData& fileData_)
		: SSystemRequestDataBase(ESystemRequestType::GetFileData)
		, name(szName)
		, fileData(fileData_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::GetFileData> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::GetFileData)
		, name(pAMRData->name)
		, fileData(pAMRData->fileData)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxFileNameLength> const name;
	SFileData&                               fileData;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::GetImplInfo> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(SImplInfo& implInfo_)
		: SSystemRequestDataBase(ESystemRequestType::GetImplInfo)
		, implInfo(implInfo_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::GetImplInfo> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::GetImplInfo)
		, implInfo(pAMRData->implInfo)
	{}

	virtual ~SSystemRequestData() override = default;

	SImplInfo& implInfo;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::RegisterListener> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(CListener** const ppListener_, CTransformation const& transformation_, char const* const szName)
		: SSystemRequestDataBase(ESystemRequestType::RegisterListener)
		, ppListener(ppListener_)
		, transformation(transformation_)
		, name(szName)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::RegisterListener> const* const pALRData)
		: SSystemRequestDataBase(ESystemRequestType::RegisterListener)
		, ppListener(pALRData->ppListener)
		, name(pALRData->name)
	{}

	virtual ~SSystemRequestData() override = default;

	CListener** const                          ppListener;
	CTransformation const                      transformation;
	CryFixedStringT<MaxObjectNameLength> const name;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ReleaseListener> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(CListener* const pListener_)
		: SSystemRequestDataBase(ESystemRequestType::ReleaseListener)
		, pListener(pListener_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ReleaseListener> const* const pALRData)
		: SSystemRequestDataBase(ESystemRequestType::ReleaseListener)
		, pListener(pALRData->pListener)
	{}

	virtual ~SSystemRequestData() override = default;

	CListener* const pListener;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::RegisterObject> final : public SSystemRequestDataBase, public CPoolObject<SSystemRequestData<ESystemRequestType::RegisterObject>, stl::PSyncMultiThread>
{
	explicit SSystemRequestData(CObject** const ppObject_, SCreateObjectData const& data)
		: SSystemRequestDataBase(ESystemRequestType::RegisterObject)
		, ppObject(ppObject_)
		, name(data.szName)
		, occlusionType(data.occlusionType)
		, transformation(data.transformation)
		, entityId(data.entityId)
		, setCurrentEnvironments(data.setCurrentEnvironments)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::RegisterObject> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::RegisterObject)
		, ppObject(pAMRData->ppObject)
		, name(pAMRData->name)
		, occlusionType(pAMRData->occlusionType)
		, transformation(pAMRData->transformation)
		, entityId(pAMRData->entityId)
		, setCurrentEnvironments(pAMRData->setCurrentEnvironments)
	{}

	virtual ~SSystemRequestData() override = default;

	CObject** const                            ppObject;
	CryFixedStringT<MaxObjectNameLength> const name;
	EOcclusionType const                       occlusionType;
	CTransformation const                      transformation;
	EntityId const                             entityId;
	bool const setCurrentEnvironments;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ReleaseObject> final : public SSystemRequestDataBase, public CPoolObject<SSystemRequestData<ESystemRequestType::ReleaseObject>, stl::PSyncMultiThread>
{
	explicit SSystemRequestData(CObject* const pObject_)
		: SSystemRequestDataBase(ESystemRequestType::ReleaseObject)
		, pObject(pObject_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ReleaseObject> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ReleaseObject)
		, pObject(pAMRData->pObject)
	{}

	virtual ~SSystemRequestData() override = default;

	CObject* const pObject;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ExecuteTrigger> final : public SSystemRequestDataBase, public CPoolObject<SSystemRequestData<ESystemRequestType::ExecuteTrigger>, stl::PSyncMultiThread>
{
	explicit SSystemRequestData(ControlId const triggerId_)
		: SSystemRequestDataBase(ESystemRequestType::ExecuteTrigger)
		, triggerId(triggerId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ExecuteTrigger> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::ExecuteTrigger)
		, triggerId(pASRData->triggerId)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ExecuteTriggerEx> final : public SSystemRequestDataBase, public CPoolObject<SSystemRequestData<ESystemRequestType::ExecuteTriggerEx>, stl::PSyncMultiThread>
{
	explicit SSystemRequestData(SExecuteTriggerData const& data)
		: SSystemRequestDataBase(ESystemRequestType::ExecuteTriggerEx)
		, name(data.szName)
		, occlusionType(data.occlusionType)
		, transformation(data.transformation)
		, entityId(data.entityId)
		, setCurrentEnvironments(data.setCurrentEnvironments)
		, triggerId(data.triggerId)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ExecuteTriggerEx> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ExecuteTriggerEx)
		, name(pAMRData->name)
		, occlusionType(pAMRData->occlusionType)
		, transformation(pAMRData->transformation)
		, entityId(pAMRData->entityId)
		, setCurrentEnvironments(pAMRData->setCurrentEnvironments)
		, triggerId(pAMRData->triggerId)
	{}

	virtual ~SSystemRequestData() override = default;

	CryFixedStringT<MaxObjectNameLength> const name;
	EOcclusionType const                       occlusionType;
	CTransformation const                      transformation;
	EntityId const                             entityId;
	bool const setCurrentEnvironments;
	ControlId const                            triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(EDefaultTriggerType const triggerType_)
		: SSystemRequestDataBase(ESystemRequestType::ExecuteDefaultTrigger)
		, triggerType(triggerType_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ExecuteDefaultTrigger> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ExecuteDefaultTrigger)
		, triggerType(pAMRData->triggerType)
	{}

	virtual ~SSystemRequestData() override = default;

	EDefaultTriggerType const triggerType;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::StopTrigger> final : public SSystemRequestDataBase, public CPoolObject<SSystemRequestData<ESystemRequestType::StopTrigger>, stl::PSyncMultiThread>
{
	explicit SSystemRequestData(ControlId const triggerId_)
		: SSystemRequestDataBase(ESystemRequestType::StopTrigger)
		, triggerId(triggerId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::StopTrigger> const* const pASRData)
		: SSystemRequestDataBase(ESystemRequestType::StopTrigger)
		, triggerId(pASRData->triggerId)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const triggerId;
};

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ExecutePreviewTrigger> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(ControlId const triggerId_)
		: SSystemRequestDataBase(ESystemRequestType::ExecutePreviewTrigger)
		, triggerId(triggerId_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ExecutePreviewTrigger> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ExecutePreviewTrigger)
		, triggerId(pAMRData->triggerId)
	{}

	virtual ~SSystemRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SSystemRequestData<ESystemRequestType::ExecutePreviewTriggerEx> final : public SSystemRequestDataBase
{
	explicit SSystemRequestData(Impl::ITriggerInfo const& triggerInfo_)
		: SSystemRequestDataBase(ESystemRequestType::ExecutePreviewTriggerEx)
		, triggerInfo(triggerInfo_)
	{}

	explicit SSystemRequestData(SSystemRequestData<ESystemRequestType::ExecutePreviewTriggerEx> const* const pAMRData)
		: SSystemRequestDataBase(ESystemRequestType::ExecutePreviewTriggerEx)
		, triggerInfo(pAMRData->triggerInfo)
	{}

	virtual ~SSystemRequestData() override = default;

	Impl::ITriggerInfo const& triggerInfo;
};
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
