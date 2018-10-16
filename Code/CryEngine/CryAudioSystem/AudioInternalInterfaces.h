// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CATLListener;
class CATLAudioObject;
class CAudioRayInfo;

enum class ERequestType : EnumFlagsType
{
	None,
	ManagerRequest,
	CallbackManagerRequest,
	ObjectRequest,
	ListenerRequest,
};

enum class EStandaloneFileState : EnumFlagsType
{
	None,
	Playing,
	Stopping,
	Loading,
};

enum class EManagerRequestType : EnumFlagsType
{
	None,
	SetAudioImpl,
	ReleaseAudioImpl,
	RefreshAudioSystem,
	StopAllSounds,
	ParseControlsData,
	ParsePreloadsData,
	ClearControlsData,
	ClearPreloadsData,
	PreloadSingleRequest,
	UnloadSingleRequest,
	AutoLoadSetting,
	LoadSetting,
	UnloadSetting,
	UnloadAFCMDataByScope,
	DrawDebugInfo,
	AddRequestListener,
	RemoveRequestListener,
	ChangeLanguage,
	RetriggerAudioControls,
	ReleasePendingRays,
	ReloadControlsData,
	GetAudioFileData,
	GetImplInfo,
	RegisterListener,
	ReleaseListener,
	RegisterObject,
	ReleaseObject,
	ExecuteTriggerEx,
	ExecuteDefaultTrigger,
	ExecutePreviewTrigger,
	StopPreviewTrigger,
};

enum class ECallbackManagerRequestType : EnumFlagsType
{
	None,
	ReportStartedEvent, //!< Only relevant for delayed playback.
	ReportFinishedEvent,
	ReportFinishedTriggerInstance,
	ReportStartedFile,
	ReportStoppedFile,
	ReportVirtualizedEvent,
	ReportPhysicalizedEvent,
};

enum class EObjectRequestType : EnumFlagsType
{
	None,
	LoadTrigger,
	UnloadTrigger,
	PlayFile,
	StopFile,
	ExecuteTrigger,
	StopTrigger,
	StopAllTriggers,
	SetTransformation,
	SetOcclusionType,
	SetOcclusionRayOffset,
	SetParameter,
	SetSwitchState,
	SetCurrentEnvironments,
	SetEnvironment,
	ProcessPhysicsRay,
	SetName,
	ToggleAbsoluteVelocityTracking,
	ToggleRelativeVelocityTracking,
};

enum class EListenerRequestType : EnumFlagsType
{
	None,
	SetTransformation,
	SetName,
};

enum class EOcclusionCollisionType : EnumFlagsType
{
	None    = 0,
	Static  = BIT(6),  // a,
	Rigid   = BIT(7),  // b,
	Water   = BIT(8),  // c,
	Terrain = BIT(9),  // d,
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EOcclusionCollisionType);

//////////////////////////////////////////////////////////////////////////
struct SRequestData : public _i_multithread_reference_target_t
{
	explicit SRequestData(ERequestType const requestType_)
		: requestType(requestType_)
	{}

	virtual ~SRequestData() override = default;

	SRequestData(SRequestData const&) = delete;
	SRequestData(SRequestData&&) = delete;
	SRequestData& operator=(SRequestData const&) = delete;
	SRequestData& operator=(SRequestData&&) = delete;

	ERequestType const requestType;
};

//////////////////////////////////////////////////////////////////////////
struct SManagerRequestDataBase : public SRequestData
{
	explicit SManagerRequestDataBase(EManagerRequestType const managerRequestType_)
		: SRequestData(ERequestType::ManagerRequest)
		, managerRequestType(managerRequestType_)
	{}

	virtual ~SManagerRequestDataBase() override = default;

	EManagerRequestType const managerRequestType;
};

//////////////////////////////////////////////////////////////////////////
template<EManagerRequestType T>
struct SManagerRequestData final : public SManagerRequestDataBase
{
	SManagerRequestData()
		: SManagerRequestDataBase(T)
	{}

	explicit SManagerRequestData(SManagerRequestData<T> const* const pAMRData)
		: SManagerRequestDataBase(T)
	{}

	virtual ~SManagerRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::SetAudioImpl> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(Impl::IImpl* const pIImpl_)
		: SManagerRequestDataBase(EManagerRequestType::SetAudioImpl)
		, pIImpl(pIImpl_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::SetAudioImpl> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::SetAudioImpl)
		, pIImpl(pAMRData->pIImpl)
	{}

	virtual ~SManagerRequestData() override = default;

	Impl::IImpl* const pIImpl;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::AddRequestListener> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(
		void const* const pObjectToListenTo_,
		void (*func_)(SRequestInfo const* const),
		ESystemEvents const eventMask_)
		: SManagerRequestDataBase(EManagerRequestType::AddRequestListener)
		, pObjectToListenTo(pObjectToListenTo_)
		, func(func_)
		, eventMask(eventMask_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::AddRequestListener> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::AddRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
		, eventMask(pAMRData->eventMask)
	{}

	virtual ~SManagerRequestData() override = default;

	void const* const   pObjectToListenTo;
	void                (* func)(SRequestInfo const* const);
	ESystemEvents const eventMask;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::RemoveRequestListener> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(void const* const pObjectToListenTo_, void (*func_)(SRequestInfo const* const))
		: SManagerRequestDataBase(EManagerRequestType::RemoveRequestListener)
		, pObjectToListenTo(pObjectToListenTo_)
		, func(func_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::RemoveRequestListener> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::RemoveRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
	{}

	virtual ~SManagerRequestData() override = default;

	void const* const pObjectToListenTo;
	void              (* func)(SRequestInfo const* const);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ParseControlsData> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(char const* const szFolderPath_, EDataScope const dataScope_)
		: SManagerRequestDataBase(EManagerRequestType::ParseControlsData)
		, folderPath(szFolderPath_)
		, dataScope(dataScope_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ParseControlsData> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ParseControlsData)
		, folderPath(pAMRData->folderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SManagerRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	EDataScope const                         dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ParsePreloadsData> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(char const* const szFolderPath_, EDataScope const dataScope_)
		: SManagerRequestDataBase(EManagerRequestType::ParsePreloadsData)
		, folderPath(szFolderPath_)
		, dataScope(dataScope_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ParsePreloadsData> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ParsePreloadsData)
		, folderPath(pAMRData->folderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SManagerRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	EDataScope const                         dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ClearControlsData> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(EDataScope const dataScope_)
		: SManagerRequestDataBase(EManagerRequestType::ClearControlsData)
		, dataScope(dataScope_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ClearControlsData> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ClearControlsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SManagerRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ClearPreloadsData> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(EDataScope const dataScope_)
		: SManagerRequestDataBase(EManagerRequestType::ClearPreloadsData)
		, dataScope(dataScope_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ClearPreloadsData> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ClearPreloadsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SManagerRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::PreloadSingleRequest> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(PreloadRequestId const audioPreloadRequestId_, bool const bAutoLoadOnly_)
		: SManagerRequestDataBase(EManagerRequestType::PreloadSingleRequest)
		, audioPreloadRequestId(audioPreloadRequestId_)
		, bAutoLoadOnly(bAutoLoadOnly_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::PreloadSingleRequest> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::PreloadSingleRequest)
		, audioPreloadRequestId(pAMRData->audioPreloadRequestId)
		, bAutoLoadOnly(pAMRData->bAutoLoadOnly)
	{}

	virtual ~SManagerRequestData() override = default;

	PreloadRequestId const audioPreloadRequestId;
	bool const             bAutoLoadOnly;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::UnloadSingleRequest> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(PreloadRequestId const audioPreloadRequestId_)
		: SManagerRequestDataBase(EManagerRequestType::UnloadSingleRequest)
		, audioPreloadRequestId(audioPreloadRequestId_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::UnloadSingleRequest> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::UnloadSingleRequest)
		, audioPreloadRequestId(pAMRData->audioPreloadRequestId)
	{}

	virtual ~SManagerRequestData() override = default;

	PreloadRequestId const audioPreloadRequestId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::AutoLoadSetting> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(EDataScope const scope_)
		: SManagerRequestDataBase(EManagerRequestType::AutoLoadSetting)
		, scope(scope_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::AutoLoadSetting> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::AutoLoadSetting)
		, scope(pAMRData->scope)
	{}

	virtual ~SManagerRequestData() override = default;

	EDataScope const scope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::LoadSetting> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(ControlId const id_)
		: SManagerRequestDataBase(EManagerRequestType::LoadSetting)
		, id(id_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::LoadSetting> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::LoadSetting)
		, id(pAMRData->id)
	{}

	virtual ~SManagerRequestData() override = default;

	ControlId const id;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::UnloadSetting> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(ControlId const id_)
		: SManagerRequestDataBase(EManagerRequestType::UnloadSetting)
		, id(id_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::UnloadSetting> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::UnloadSetting)
		, id(pAMRData->id)
	{}

	virtual ~SManagerRequestData() override = default;

	ControlId const id;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::UnloadAFCMDataByScope> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(EDataScope const dataScope_)
		: SManagerRequestDataBase(EManagerRequestType::UnloadAFCMDataByScope)
		, dataScope(dataScope_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::UnloadAFCMDataByScope> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::UnloadAFCMDataByScope)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SManagerRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::RefreshAudioSystem> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(char const* const szLevelName)
		: SManagerRequestDataBase(EManagerRequestType::RefreshAudioSystem)
		, levelName(szLevelName)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::RefreshAudioSystem> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::RefreshAudioSystem)
		, levelName(pAMRData->levelName)
	{}

	virtual ~SManagerRequestData() override = default;

	CryFixedStringT<MaxFileNameLength> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ReloadControlsData> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(char const* const szFolderPath, char const* const szLevelName)
		: SManagerRequestDataBase(EManagerRequestType::ReloadControlsData)
		, folderPath(szFolderPath)
		, levelName(szLevelName)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ReloadControlsData> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ReloadControlsData)
		, folderPath(pAMRData->folderPath)
		, levelName(pAMRData->levelName)
	{}

	virtual ~SManagerRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	CryFixedStringT<MaxFilePathLength> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::GetAudioFileData> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(char const* const szName, SFileData& fileData_)
		: SManagerRequestDataBase(EManagerRequestType::GetAudioFileData)
		, name(szName)
		, fileData(fileData_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::GetAudioFileData> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::GetAudioFileData)
		, name(pAMRData->name)
		, fileData(pAMRData->fileData)
	{}

	virtual ~SManagerRequestData() override = default;

	CryFixedStringT<MaxFileNameLength> const name;
	SFileData&                               fileData;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::GetImplInfo> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(SImplInfo& implInfo_)
		: SManagerRequestDataBase(EManagerRequestType::GetImplInfo)
		, implInfo(implInfo_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::GetImplInfo> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::GetImplInfo)
		, implInfo(pAMRData->implInfo)
	{}

	virtual ~SManagerRequestData() override = default;

	SImplInfo& implInfo;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::RegisterListener> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(CATLListener** const ppListener_, CObjectTransformation const& transformation_, char const* const szName)
		: SManagerRequestDataBase(EManagerRequestType::RegisterListener)
		, ppListener(ppListener_)
		, transformation(transformation_)
		, name(szName)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::RegisterListener> const* const pALRData)
		: SManagerRequestDataBase(EManagerRequestType::RegisterListener)
		, ppListener(pALRData->ppListener)
		, name(pALRData->name)
	{}

	virtual ~SManagerRequestData() override = default;

	CATLListener** const                       ppListener;
	CObjectTransformation const                transformation;
	CryFixedStringT<MaxObjectNameLength> const name;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ReleaseListener> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(CATLListener* const pListener_)
		: SManagerRequestDataBase(EManagerRequestType::ReleaseListener)
		, pListener(pListener_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ReleaseListener> const* const pALRData)
		: SManagerRequestDataBase(EManagerRequestType::ReleaseListener)
		, pListener(pALRData->pListener)
	{}

	virtual ~SManagerRequestData() override = default;

	CATLListener* const pListener;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::RegisterObject> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(CATLAudioObject** const ppObject_, SCreateObjectData const& data)
		: SManagerRequestDataBase(EManagerRequestType::RegisterObject)
		, ppObject(ppObject_)
		, name(data.szName)
		, occlusionType(data.occlusionType)
		, transformation(data.transformation)
		, entityId(data.entityId)
		, setCurrentEnvironments(data.setCurrentEnvironments)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::RegisterObject> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::RegisterObject)
		, ppObject(pAMRData->ppObject)
		, name(pAMRData->name)
		, occlusionType(pAMRData->occlusionType)
		, transformation(pAMRData->transformation)
		, entityId(pAMRData->entityId)
		, setCurrentEnvironments(pAMRData->setCurrentEnvironments)
	{}

	virtual ~SManagerRequestData() override = default;

	CATLAudioObject** const                    ppObject;
	CryFixedStringT<MaxObjectNameLength> const name;
	EOcclusionType const                       occlusionType;
	CObjectTransformation const                transformation;
	EntityId const                             entityId;
	bool const setCurrentEnvironments;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ReleaseObject> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(CATLAudioObject* const pObject_)
		: SManagerRequestDataBase(EManagerRequestType::ReleaseObject)
		, pObject(pObject_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ReleaseObject> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ReleaseObject)
		, pObject(pAMRData->pObject)
	{}

	virtual ~SManagerRequestData() override = default;

	CATLAudioObject* const pObject;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ExecuteTriggerEx> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(SExecuteTriggerData const& data)
		: SManagerRequestDataBase(EManagerRequestType::ExecuteTriggerEx)
		, name(data.szName)
		, occlusionType(data.occlusionType)
		, transformation(data.transformation)
		, entityId(data.entityId)
		, setCurrentEnvironments(data.setCurrentEnvironments)
		, triggerId(data.triggerId)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ExecuteTriggerEx> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ExecuteTriggerEx)
		, name(pAMRData->name)
		, occlusionType(pAMRData->occlusionType)
		, transformation(pAMRData->transformation)
		, entityId(pAMRData->entityId)
		, setCurrentEnvironments(pAMRData->setCurrentEnvironments)
		, triggerId(pAMRData->triggerId)
	{}

	virtual ~SManagerRequestData() override = default;

	CryFixedStringT<MaxObjectNameLength> const name;
	EOcclusionType const                       occlusionType;
	CObjectTransformation const                transformation;
	EntityId const                             entityId;
	bool const setCurrentEnvironments;
	ControlId const                            triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ExecuteDefaultTrigger> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(EDefaultTriggerType const triggerType_)
		: SManagerRequestDataBase(EManagerRequestType::ExecuteDefaultTrigger)
		, triggerType(triggerType_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ExecuteDefaultTrigger> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ExecuteDefaultTrigger)
		, triggerType(pAMRData->triggerType)
	{}

	virtual ~SManagerRequestData() override = default;

	EDefaultTriggerType const triggerType;
};

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
template<>
struct SManagerRequestData<EManagerRequestType::ExecutePreviewTrigger> final : public SManagerRequestDataBase
{
	explicit SManagerRequestData(Impl::ITriggerInfo const& triggerInfo_)
		: SManagerRequestDataBase(EManagerRequestType::ExecutePreviewTrigger)
		, triggerInfo(triggerInfo_)
	{}

	explicit SManagerRequestData(SManagerRequestData<EManagerRequestType::ExecutePreviewTrigger> const* const pAMRData)
		: SManagerRequestDataBase(EManagerRequestType::ExecutePreviewTrigger)
		, triggerInfo(pAMRData->triggerInfo)
	{}

	virtual ~SManagerRequestData() override = default;

	Impl::ITriggerInfo const& triggerInfo;
};
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
struct SCallbackManagerRequestDataBase : public SRequestData
{
	explicit SCallbackManagerRequestDataBase(ECallbackManagerRequestType const callbackManagerRequestType_)
		: SRequestData(ERequestType::CallbackManagerRequest)
		, callbackManagerRequestType(callbackManagerRequestType_)
	{}

	virtual ~SCallbackManagerRequestDataBase() override = default;

	ECallbackManagerRequestType const callbackManagerRequestType;
};

//////////////////////////////////////////////////////////////////////////
template<ECallbackManagerRequestType T>
struct SCallbackManagerRequestData final : public SCallbackManagerRequestDataBase
{
	SCallbackManagerRequestData()
		: SCallbackManagerRequestDataBase(T)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<T> const* const pACMRData)
		: SCallbackManagerRequestDataBase(T)
	{}

	virtual ~SCallbackManagerRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedEvent> final : public SCallbackManagerRequestDataBase
{
	explicit SCallbackManagerRequestData(CATLEvent& audioEvent_, bool const isVirtual_)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportStartedEvent)
		, audioEvent(audioEvent_)
		, isVirtual(isVirtual_)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedEvent> const* const pACMRData)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportStartedEvent)
		, audioEvent(pACMRData->audioEvent)
		, isVirtual(pACMRData->isVirtual)
	{}

	virtual ~SCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
	bool       isVirtual;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedEvent> final : public SCallbackManagerRequestDataBase
{
	explicit SCallbackManagerRequestData(CATLEvent& audioEvent_, bool const bSuccess_)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportFinishedEvent)
		, audioEvent(audioEvent_)
		, bSuccess(bSuccess_)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedEvent> const* const pACMRData)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportFinishedEvent)
		, audioEvent(pACMRData->audioEvent)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
	bool const bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackManagerRequestData<ECallbackManagerRequestType::ReportVirtualizedEvent> final : public SCallbackManagerRequestDataBase
{
	explicit SCallbackManagerRequestData(CATLEvent& audioEvent_)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportVirtualizedEvent)
		, audioEvent(audioEvent_)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<ECallbackManagerRequestType::ReportVirtualizedEvent> const* const pACMRData)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportVirtualizedEvent)
		, audioEvent(pACMRData->audioEvent)
	{}

	virtual ~SCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackManagerRequestData<ECallbackManagerRequestType::ReportPhysicalizedEvent> final : public SCallbackManagerRequestDataBase
{
	explicit SCallbackManagerRequestData(CATLEvent& audioEvent_)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportPhysicalizedEvent)
		, audioEvent(audioEvent_)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<ECallbackManagerRequestType::ReportPhysicalizedEvent> const* const pACMRData)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportPhysicalizedEvent)
		, audioEvent(pACMRData->audioEvent)
	{}

	virtual ~SCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedTriggerInstance> final : public SCallbackManagerRequestDataBase
{
	explicit SCallbackManagerRequestData(ControlId const audioTriggerId_)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportFinishedTriggerInstance)
		, audioTriggerId(audioTriggerId_)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<ECallbackManagerRequestType::ReportFinishedTriggerInstance> const* const pACMRData)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportFinishedTriggerInstance)
		, audioTriggerId(pACMRData->audioTriggerId)
	{}

	virtual ~SCallbackManagerRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedFile> final : public SCallbackManagerRequestDataBase
{
	explicit SCallbackManagerRequestData(CATLStandaloneFile& audioStandaloneFile_, bool const bSuccess_)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportStartedFile)
		, audioStandaloneFile(audioStandaloneFile_)
		, bSuccess(bSuccess_)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStartedFile> const* const pACMRData)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportStartedFile)
		, audioStandaloneFile(pACMRData->audioStandaloneFile)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SCallbackManagerRequestData() override = default;

	CATLStandaloneFile& audioStandaloneFile;
	bool const          bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStoppedFile> final : public SCallbackManagerRequestDataBase
{
	explicit SCallbackManagerRequestData(CATLStandaloneFile& audioStandaloneFile_)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportStoppedFile)
		, audioStandaloneFile(audioStandaloneFile_)
	{}

	explicit SCallbackManagerRequestData(SCallbackManagerRequestData<ECallbackManagerRequestType::ReportStoppedFile> const* const pACMRData)
		: SCallbackManagerRequestDataBase(ECallbackManagerRequestType::ReportStoppedFile)
		, audioStandaloneFile(pACMRData->audioStandaloneFile)
	{}

	virtual ~SCallbackManagerRequestData() override = default;

	CATLStandaloneFile& audioStandaloneFile;
};

//////////////////////////////////////////////////////////////////////////
struct SObjectRequestDataBase : public SRequestData
{
	explicit SObjectRequestDataBase(EObjectRequestType const objectRequestType_)
		: SRequestData(ERequestType::ObjectRequest)
		, objectRequestType(objectRequestType_)
	{}

	virtual ~SObjectRequestDataBase() override = default;

	EObjectRequestType const objectRequestType;
};

//////////////////////////////////////////////////////////////////////////
template<EObjectRequestType T>
struct SObjectRequestData final : public SObjectRequestDataBase
{
	SObjectRequestData()
		: SObjectRequestDataBase(T)
	{}

	explicit SObjectRequestData(SObjectRequestData<T> const* const pAORData)
		: SObjectRequestDataBase(T)
	{}

	virtual ~SObjectRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::LoadTrigger> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(ControlId const audioTriggerId_)
		: SObjectRequestDataBase(EObjectRequestType::LoadTrigger)
		, audioTriggerId(audioTriggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::LoadTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::LoadTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::UnloadTrigger> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(ControlId const audioTriggerId_)
		: SObjectRequestDataBase(EObjectRequestType::UnloadTrigger)
		, audioTriggerId(audioTriggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::UnloadTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::UnloadTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::PlayFile> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(
		CryFixedStringT<MaxFilePathLength> const& file_,
		ControlId const usedAudioTriggerId_,
		bool const bLocalized_)
		: SObjectRequestDataBase(EObjectRequestType::PlayFile)
		, file(file_)
		, usedAudioTriggerId(usedAudioTriggerId_)
		, bLocalized(bLocalized_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::PlayFile> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::PlayFile)
		, file(pAORData->file)
		, usedAudioTriggerId(pAORData->usedAudioTriggerId)
		, bLocalized(pAORData->bLocalized)
	{}

	virtual ~SObjectRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
	ControlId const                          usedAudioTriggerId;
	bool const                               bLocalized;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::StopFile> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CryFixedStringT<MaxFilePathLength> const& file_)
		: SObjectRequestDataBase(EObjectRequestType::StopFile)
		, file(file_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::StopFile> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::StopFile)
		, file(pAORData->file)
	{}

	virtual ~SObjectRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ExecuteTrigger> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(ControlId const audioTriggerId_)
		: SObjectRequestDataBase(EObjectRequestType::ExecuteTrigger)
		, audioTriggerId(audioTriggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ExecuteTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::ExecuteTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::StopTrigger> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(ControlId const audioTriggerId_)
		: SObjectRequestDataBase(EObjectRequestType::StopTrigger)
		, audioTriggerId(audioTriggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::StopTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::StopTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetTransformation> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObjectTransformation const& transformation_)
		: SObjectRequestDataBase(EObjectRequestType::SetTransformation)
		, transformation(transformation_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetTransformation> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetTransformation)
		, transformation(pAORData->transformation)
	{}

	virtual ~SObjectRequestData() override = default;

	CObjectTransformation const transformation;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetOcclusionType> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(EOcclusionType const occlusionType_)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionType)
		, occlusionType(occlusionType_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetOcclusionType> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionType)
		, occlusionType(pAORData->occlusionType)
	{}

	virtual ~SObjectRequestData() override = default;

	EOcclusionType const occlusionType;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(float const occlusionRayOffset_)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionRayOffset)
		, occlusionRayOffset(occlusionRayOffset_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionRayOffset)
		, occlusionRayOffset(pAORData->occlusionRayOffset)
	{}

	virtual ~SObjectRequestData() override = default;

	float const occlusionRayOffset;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetParameter> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(ControlId const parameterId_, float const value_)
		: SObjectRequestDataBase(EObjectRequestType::SetParameter)
		, parameterId(parameterId_)
		, value(value_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetParameter> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetParameter)
		, parameterId(pAORData->parameterId)
		, value(pAORData->value)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const parameterId;
	float const     value;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetSwitchState> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(ControlId const audioSwitchId_, SwitchStateId const audioSwitchStateId_)
		: SObjectRequestDataBase(EObjectRequestType::SetSwitchState)
		, audioSwitchId(audioSwitchId_)
		, audioSwitchStateId(audioSwitchStateId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetSwitchState> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetSwitchState)
		, audioSwitchId(pAORData->audioSwitchId)
		, audioSwitchStateId(pAORData->audioSwitchStateId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const     audioSwitchId;
	SwitchStateId const audioSwitchStateId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(EntityId const entityToIgnore_)
		: SObjectRequestDataBase(EObjectRequestType::SetCurrentEnvironments)
		, entityToIgnore(entityToIgnore_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetCurrentEnvironments)
		, entityToIgnore(pAORData->entityToIgnore)
	{}

	virtual ~SObjectRequestData() override = default;

	EntityId const entityToIgnore;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetEnvironment> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(EnvironmentId const audioEnvironmentId_, float const amount_)
		: SObjectRequestDataBase(EObjectRequestType::SetEnvironment)
		, audioEnvironmentId(audioEnvironmentId_)
		, amount(amount_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetEnvironment> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetEnvironment)
		, audioEnvironmentId(pAORData->audioEnvironmentId)
		, amount(pAORData->amount)
	{}

	virtual ~SObjectRequestData() override = default;

	EnvironmentId const audioEnvironmentId;
	float const         amount;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CAudioRayInfo* const pAudioRayInfo_)
		: SObjectRequestDataBase(EObjectRequestType::ProcessPhysicsRay)
		, pAudioRayInfo(pAudioRayInfo_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::ProcessPhysicsRay)
		, pAudioRayInfo(pAORData->pAudioRayInfo)
	{}

	virtual ~SObjectRequestData() override = default;

	CAudioRayInfo* const pAudioRayInfo;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetName> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(char const* const szName)
		: SObjectRequestDataBase(EObjectRequestType::SetName)
		, name(szName)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetName> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetName)
		, name(pAORData->name)
	{}

	virtual ~SObjectRequestData() override = default;

	CryFixedStringT<MaxObjectNameLength> const name;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(bool const isEnabled_)
		: SObjectRequestDataBase(EObjectRequestType::ToggleAbsoluteVelocityTracking)
		, isEnabled(isEnabled_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::ToggleAbsoluteVelocityTracking)
		, isEnabled(pAORData->isEnabled)
	{}

	virtual ~SObjectRequestData() override = default;

	bool const isEnabled;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(bool const isEnabled_)
		: SObjectRequestDataBase(EObjectRequestType::ToggleRelativeVelocityTracking)
		, isEnabled(isEnabled_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::ToggleRelativeVelocityTracking)
		, isEnabled(pAORData->isEnabled)
	{}

	virtual ~SObjectRequestData() override = default;

	bool const isEnabled;
};

//////////////////////////////////////////////////////////////////////////
struct SListenerRequestDataBase : public SRequestData
{
	explicit SListenerRequestDataBase(EListenerRequestType const listenerRequestType_)
		: SRequestData(ERequestType::ListenerRequest)
		, listenerRequestType(listenerRequestType_)
	{}

	virtual ~SListenerRequestDataBase() override = default;

	EListenerRequestType const listenerRequestType;
};

//////////////////////////////////////////////////////////////////////////
template<EListenerRequestType T>
struct SListenerRequestData final : public SListenerRequestDataBase
{
	SListenerRequestData()
		: SListenerRequestDataBase(T)
	{}

	explicit SListenerRequestData(SListenerRequestData<T> const* const pALRData)
		: SListenerRequestDataBase(T)
	{}

	virtual ~SListenerRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SListenerRequestData<EListenerRequestType::SetTransformation> final : public SListenerRequestDataBase
{
	explicit SListenerRequestData(
		CObjectTransformation const& transformation_,
		CATLListener* const pListener_)
		: SListenerRequestDataBase(EListenerRequestType::SetTransformation)
		, transformation(transformation_)
		, pListener(pListener_)
	{}

	explicit SListenerRequestData(SListenerRequestData<EListenerRequestType::SetTransformation> const* const pALRData)
		: SListenerRequestDataBase(EListenerRequestType::SetTransformation)
		, transformation(pALRData->transformation)
		, pListener(pALRData->pListener)
	{}

	virtual ~SListenerRequestData() override = default;

	CObjectTransformation const transformation;
	CATLListener* const         pListener;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SListenerRequestData<EListenerRequestType::SetName> final : public SListenerRequestDataBase
{
	explicit SListenerRequestData(char const* const szName, CATLListener* const pListener_)
		: SListenerRequestDataBase(EListenerRequestType::SetName)
		, pListener(pListener_)
		, name(szName)
	{}

	explicit SListenerRequestData(SListenerRequestData<EListenerRequestType::SetName> const* const pALRData)
		: SListenerRequestDataBase(EListenerRequestType::SetName)
		, pListener(pALRData->pListener)
		, name(pALRData->name)
	{}

	virtual ~SListenerRequestData() override = default;

	CATLListener* const                        pListener;
	CryFixedStringT<MaxObjectNameLength> const name;
};

SRequestData* AllocateRequestData(SRequestData const* const pRequestData);

class CAudioRequest
{
public:

	CAudioRequest() = default;

	explicit CAudioRequest(SRequestData const* const pRequestData)
		: flags(ERequestFlags::None)
		, pObject(nullptr)
		, pOwner(nullptr)
		, pUserData(nullptr)
		, pUserDataOwner(nullptr)
		, status(ERequestStatus::None)
		, pData(AllocateRequestData(pRequestData))
	{}

	explicit CAudioRequest(
		ERequestFlags const flags_,
		CATLAudioObject* const pObject_,
		void* const pOwner_,
		void* const pUserData_,
		void* const pUserDataOwner_,
		SRequestData const* const pRequestData_)
		: flags(flags_)
		, pObject(pObject_)
		, pOwner(pOwner_)
		, pUserData(pUserData_)
		, pUserDataOwner(pUserDataOwner_)
		, status(ERequestStatus::None)
		, pData(AllocateRequestData(pRequestData_))
	{}

	SRequestData* GetData() const { return pData.get(); }

	ERequestFlags    flags = ERequestFlags::None;
	CATLAudioObject* pObject = nullptr;
	void*            pOwner = nullptr;
	void*            pUserData = nullptr;
	void*            pUserDataOwner = nullptr;
	ERequestStatus   status = ERequestStatus::None;

private:

	// Must be private as it needs "AllocateRequestData"!
	_smart_ptr<SRequestData> pData = nullptr;
};

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
namespace Debug
{
// Filter for drawing debug info to the screen
enum class EDrawFilter : EnumFlagsType
{
	All                    = 0,
	Spheres                = BIT(6),  // a
	ObjectLabel            = BIT(7),  // b
	ObjectTriggers         = BIT(8),  // c
	ObjectStates           = BIT(9),  // d
	ObjectParameters       = BIT(10), // e
	ObjectEnvironments     = BIT(11), // f
	ObjectDistance         = BIT(12), // g
	OcclusionRayLabels     = BIT(13), // h
	OcclusionRays          = BIT(14), // i
	OcclusionRayOffset     = BIT(15), // j
	ListenerOcclusionPlane = BIT(16), // k
	ObjectStandaloneFiles  = BIT(17), // l
	ObjectImplInfo         = BIT(18), // m

	HideMemoryInfo         = BIT(22), // q
	FilterAllObjectInfo    = BIT(23), // r
	DetailedMemoryInfo     = BIT(24), // s

	StandaloneFiles        = BIT(26), // u
	ActiveEvents           = BIT(27), // v
	ActiveObjects          = BIT(28), // w
	FileCacheManagerInfo   = BIT(29), // x
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EDrawFilter);

static constexpr EDrawFilter objectMask =
	EDrawFilter::Spheres |
	EDrawFilter::ObjectLabel |
	EDrawFilter::ObjectTriggers |
	EDrawFilter::ObjectStates |
	EDrawFilter::ObjectParameters |
	EDrawFilter::ObjectEnvironments |
	EDrawFilter::ObjectDistance |
	EDrawFilter::OcclusionRayLabels |
	EDrawFilter::OcclusionRays |
	EDrawFilter::OcclusionRayOffset |
	EDrawFilter::ListenerOcclusionPlane |
	EDrawFilter::ObjectStandaloneFiles |
	EDrawFilter::ObjectImplInfo;
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

} // namespace CryAudio
