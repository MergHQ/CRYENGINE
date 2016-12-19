// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>
#include <CryAudio/IAudioSystem.h>
#include <CryString/CryName.h>

namespace CryAudio
{
class CATLListener;
class CATLAudioObject;
class CAudioRayInfo;

enum EAudioRequestType : EnumFlagsType
{
	eAudioRequestType_None,
	eAudioRequestType_AudioManagerRequest,
	eAudioRequestType_AudioCallbackManagerRequest,
	eAudioRequestType_AudioObjectRequest,
	eAudioRequestType_AudioListenerRequest,
};

struct SAudioRequestDataBase
{
	explicit SAudioRequestDataBase(EAudioRequestType const _type)
		: type(_type)
	{}

	virtual ~SAudioRequestDataBase() = default;

	SAudioRequestDataBase(SAudioRequestDataBase const&) = delete;
	SAudioRequestDataBase(SAudioRequestDataBase&&) = delete;
	SAudioRequestDataBase& operator=(SAudioRequestDataBase const&) = delete;
	SAudioRequestDataBase& operator=(SAudioRequestDataBase&&) = delete;

	EAudioRequestType const type;
};

enum EAudioRequestInfoFlags : EnumFlagsType
{
	eAudioRequestInfoFlags_None              = 0,
	eAudioRequestInfoFlags_WaitingForRemoval = BIT(0),
};

enum EAudioStandaloneFileState : EnumFlagsType
{
	eAudioStandaloneFileState_None,
	eAudioStandaloneFileState_Playing,
	eAudioStandaloneFileState_Stopping,
	eAudioStandaloneFileState_Loading,
};

enum EAudioManagerRequestType : EnumFlagsType
{
	eAudioManagerRequestType_None                   = 0,
	eAudioManagerRequestType_SetAudioImpl           = BIT(0),
	eAudioManagerRequestType_ReleaseAudioImpl       = BIT(1),
	eAudioManagerRequestType_RefreshAudioSystem     = BIT(2),
	eAudioManagerRequestType_ConstructAudioListener = BIT(3),
	eAudioManagerRequestType_ConstructAudioObject   = BIT(4),
	eAudioManagerRequestType_LoseFocus              = BIT(5),
	eAudioManagerRequestType_GetFocus               = BIT(6),
	eAudioManagerRequestType_MuteAll                = BIT(7),
	eAudioManagerRequestType_UnmuteAll              = BIT(8),
	eAudioManagerRequestType_StopAllSounds          = BIT(9),
	eAudioManagerRequestType_ParseControlsData      = BIT(10),
	eAudioManagerRequestType_ParsePreloadsData      = BIT(11),
	eAudioManagerRequestType_ClearControlsData      = BIT(12),
	eAudioManagerRequestType_ClearPreloadsData      = BIT(13),
	eAudioManagerRequestType_PreloadSingleRequest   = BIT(14),
	eAudioManagerRequestType_UnloadSingleRequest    = BIT(15),
	eAudioManagerRequestType_UnloadAFCMDataByScope  = BIT(16),
	eAudioManagerRequestType_DrawDebugInfo          = BIT(17),
	eAudioManagerRequestType_AddRequestListener     = BIT(18),
	eAudioManagerRequestType_RemoveRequestListener  = BIT(19),
	eAudioManagerRequestType_ChangeLanguage         = BIT(20),
	eAudioManagerRequestType_RetriggerAudioControls = BIT(21),
	eAudioManagerRequestType_ReleasePendingRays     = BIT(22),
	eAudioManagerRequestType_ReloadControlsData     = BIT(23),
	eAudioManagerRequestType_GetAudioFileData       = BIT(24),
};

enum EAudioCallbackManagerRequestType : EnumFlagsType
{
	eAudioCallbackManagerRequestType_None                          = 0,
	eAudioCallbackManagerRequestType_ReportStartedEvent            = BIT(0),   //!< Only relevant for delayed playback.
	eAudioCallbackManagerRequestType_ReportFinishedEvent           = BIT(1),
	eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance = BIT(2),
	eAudioCallbackManagerRequestType_ReportStartedFile             = BIT(3),
	eAudioCallbackManagerRequestType_ReportStoppedFile             = BIT(4),
	eAudioCallbackManagerRequestType_ReportVirtualizedEvent        = BIT(5),
	eAudioCallbackManagerRequestType_ReportPhysicalizedEvent       = BIT(6),
};

enum EAudioObjectRequestType : EnumFlagsType
{
	eAudioObjectRequestType_None                   = 0,
	eAudioObjectRequestType_LoadTrigger            = BIT(0),
	eAudioObjectRequestType_UnloadTrigger          = BIT(1),
	eAudioObjectRequestType_PlayFile               = BIT(2),
	eAudioObjectRequestType_StopFile               = BIT(3),
	eAudioObjectRequestType_ExecuteTrigger         = BIT(4),
	eAudioObjectRequestType_ExecuteTriggerEx       = BIT(5),
	eAudioObjectRequestType_StopTrigger            = BIT(6),
	eAudioObjectRequestType_StopAllTriggers        = BIT(7),
	eAudioObjectRequestType_SetTransformation      = BIT(8),
	eAudioObjectRequestType_SetParameter           = BIT(9),
	eAudioObjectRequestType_SetSwitchState         = BIT(10),
	eAudioObjectRequestType_SetCurrentEnvironments = BIT(11),
	eAudioObjectRequestType_SetEnvironment         = BIT(12),
	eAudioObjectRequestType_ResetEnvironments      = BIT(13),
	eAudioObjectRequestType_ReleaseObject          = BIT(14),
	eAudioObjectRequestType_ProcessPhysicsRay      = BIT(15),
};

enum EAudioListenerRequestType : EnumFlagsType
{
	eAudioListenerRequestType_None              = 0,
	eAudioListenerRequestType_SetTransformation = BIT(0),
	eAudioListenerRequestType_ReleaseListener   = BIT(1),
};

//////////////////////////////////////////////////////////////////////////
struct SAudioEventListener
{
	SAudioEventListener()
		: pObjectToListenTo(nullptr)
		, OnEvent(nullptr)
		, eventMask(eSystemEvent_None)
	{}

	void const*   pObjectToListenTo;
	void          (* OnEvent)(SRequestInfo const* const);
	EnumFlagsType eventMask;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioRequestData : public _i_multithread_reference_target_t
{
	explicit SAudioRequestData(EAudioRequestType const _type)
		: type(_type)
	{}

	virtual ~SAudioRequestData() override = default;

	SAudioRequestData(SAudioRequestData const&) = delete;
	SAudioRequestData(SAudioRequestData&&) = delete;
	SAudioRequestData& operator=(SAudioRequestData const&) = delete;
	SAudioRequestData& operator=(SAudioRequestData&&) = delete;

	// _i_multithread_reference_target_t
	virtual void Release() override;
	// ~_i_multithread_reference_target_t

	EAudioRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioManagerRequestDataBase : public SAudioRequestData
{
	explicit SAudioManagerRequestDataBase(EAudioManagerRequestType const _type)
		: SAudioRequestData(eAudioRequestType_AudioManagerRequest)
		, type(_type)
	{}

	virtual ~SAudioManagerRequestDataBase() override = default;

	EAudioManagerRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioManagerRequestType T>
struct SAudioManagerRequestData final : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(T)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<T> const* const pAMRData)
		: SAudioManagerRequestDataBase(T)
	{}

	virtual ~SAudioManagerRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_SetAudioImpl> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(Impl::IAudioImpl* const _pImpl)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_SetAudioImpl)
		, pImpl(_pImpl)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_SetAudioImpl> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_SetAudioImpl)
		, pImpl(pAMRData->pImpl)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	Impl::IAudioImpl* const pImpl;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_ConstructAudioListener> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(CATLListener** const _ppListener)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ConstructAudioListener)
		, ppListener(_ppListener)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_ConstructAudioListener> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ConstructAudioListener)
		, ppListener(pAMRData->ppListener)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	CATLListener** const ppListener;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_ConstructAudioObject> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(CATLAudioObject** const _ppAudioObject, SCreateObjectData const& _objectData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ConstructAudioObject)
		, ppAudioObject(_ppAudioObject)
		, objectData(_objectData)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_ConstructAudioObject> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ConstructAudioObject)
		, ppAudioObject(pAMRData->ppAudioObject)
		, objectData(pAMRData->objectData)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	CATLAudioObject** const ppAudioObject;
	SCreateObjectData const objectData;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_AddRequestListener> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(
	  void const* const _pObjectToListenTo,
	  void(*_func)(SRequestInfo const* const),
	  EnumFlagsType const _eventMask)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_AddRequestListener)
		, pObjectToListenTo(_pObjectToListenTo)
		, func(_func)
		, eventMask(_eventMask)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_AddRequestListener> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_AddRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
		, eventMask(pAMRData->eventMask)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	void const* const   pObjectToListenTo;
	void                (* func)(SRequestInfo const* const);
	EnumFlagsType const eventMask;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_RemoveRequestListener> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(void const* const _pObjectToListenTo, void(*_func)(SRequestInfo const* const))
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_RemoveRequestListener)
		, pObjectToListenTo(_pObjectToListenTo)
		, func(_func)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_RemoveRequestListener> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_RemoveRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	void const* const pObjectToListenTo;
	void              (* func)(SRequestInfo const* const);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(char const* const _szFolderPath, EDataScope const _dataScope)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ParseControlsData)
		, folderPath(_szFolderPath)
		, dataScope(_dataScope)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ParseControlsData)
		, folderPath(pAMRData->folderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	EDataScope const                         dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(char const* const _szFolderPath, EDataScope const _dataScope)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ParsePreloadsData)
		, folderPath(_szFolderPath)
		, dataScope(_dataScope)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ParsePreloadsData)
		, folderPath(pAMRData->folderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	EDataScope const                         dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_ClearControlsData> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(EDataScope const _dataScope)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ClearControlsData)
		, dataScope(_dataScope)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_ClearControlsData> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ClearControlsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_ClearPreloadsData> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(EDataScope const _dataScope)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ClearPreloadsData)
		, dataScope(_dataScope)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_ClearPreloadsData> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ClearPreloadsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(PreloadRequestId const _audioPreloadRequestId, bool const _bAutoLoadOnly)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_PreloadSingleRequest)
		, audioPreloadRequestId(_audioPreloadRequestId)
		, bAutoLoadOnly(_bAutoLoadOnly)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_PreloadSingleRequest)
		, audioPreloadRequestId(pAMRData->audioPreloadRequestId)
		, bAutoLoadOnly(pAMRData->bAutoLoadOnly)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	PreloadRequestId const audioPreloadRequestId;
	bool const             bAutoLoadOnly;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_UnloadSingleRequest> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(PreloadRequestId const _audioPreloadRequestId)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_UnloadSingleRequest)
		, audioPreloadRequestId(_audioPreloadRequestId)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_UnloadSingleRequest> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_UnloadSingleRequest)
		, audioPreloadRequestId(pAMRData->audioPreloadRequestId)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	PreloadRequestId const audioPreloadRequestId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_UnloadAFCMDataByScope> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(EDataScope const _dataScope)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_UnloadAFCMDataByScope)
		, dataScope(_dataScope)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_UnloadAFCMDataByScope> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_UnloadAFCMDataByScope)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	EDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_RefreshAudioSystem> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(char const* const szLevelName)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_RefreshAudioSystem)
		, levelName(szLevelName)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_RefreshAudioSystem> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_RefreshAudioSystem)
		, levelName(pAMRData->levelName)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	CryFixedStringT<MaxFileNameLength> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_ReloadControlsData> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(char const* const szFolderPath, char const* const szLevelName)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ReloadControlsData)
		, folderPath(szFolderPath)
		, levelName(szLevelName)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_ReloadControlsData> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_ReloadControlsData)
		, folderPath(pAMRData->folderPath)
		, levelName(pAMRData->levelName)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const folderPath;
	CryFixedStringT<MaxFilePathLength> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAudioManagerRequestType_GetAudioFileData> final : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(char const* const _szFilename, SFileData& _audioFileData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_GetAudioFileData)
		, szFilename(_szFilename)
		, audioFileData(_audioFileData)
	{}

	explicit SAudioManagerRequestData(SAudioManagerRequestData<eAudioManagerRequestType_GetAudioFileData> const* const pAMRData)
		: SAudioManagerRequestDataBase(eAudioManagerRequestType_GetAudioFileData)
		, szFilename(pAMRData->szFilename)
		, audioFileData(pAMRData->audioFileData)
	{}

	virtual ~SAudioManagerRequestData() override = default;

	char const* const szFilename;
	SFileData&        audioFileData;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioCallbackManagerRequestDataBase : public SAudioRequestData
{
	explicit SAudioCallbackManagerRequestDataBase(EAudioCallbackManagerRequestType const _type)
		: SAudioRequestData(eAudioRequestType_AudioCallbackManagerRequest)
		, type(_type)
	{}

	virtual ~SAudioCallbackManagerRequestDataBase() override = default;

	EAudioCallbackManagerRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioCallbackManagerRequestType T>
struct SAudioCallbackManagerRequestData final : public SAudioCallbackManagerRequestDataBase
{
	SAudioCallbackManagerRequestData()
		: SAudioCallbackManagerRequestDataBase(T)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<T> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(T)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedEvent> final : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(CATLEvent& _audioEvent)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportStartedEvent)
		, audioEvent(_audioEvent)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportStartedEvent)
		, audioEvent(pACMRData->audioEvent)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedEvent> final : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(CATLEvent& _audioEvent, bool const _bSuccess)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportFinishedEvent)
		, audioEvent(_audioEvent)
		, bSuccess(_bSuccess)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportFinishedEvent)
		, audioEvent(pACMRData->audioEvent)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
	bool const bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportVirtualizedEvent> final : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(CATLEvent& _audioEvent)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportVirtualizedEvent)
		, audioEvent(_audioEvent)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportVirtualizedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportVirtualizedEvent)
		, audioEvent(pACMRData->audioEvent)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportPhysicalizedEvent> final : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(CATLEvent& _audioEvent)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportPhysicalizedEvent)
		, audioEvent(_audioEvent)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportPhysicalizedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportPhysicalizedEvent)
		, audioEvent(pACMRData->audioEvent)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;

	CATLEvent& audioEvent;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> final : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(ControlId const _audioTriggerId)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance)
		, audioTriggerId(_audioTriggerId)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance)
		, audioTriggerId(pACMRData->audioTriggerId)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> final : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(CATLStandaloneFile& _audioStandaloneFile, bool const _bSuccess)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportStartedFile)
		, audioStandaloneFile(_audioStandaloneFile)
		, bSuccess(_bSuccess)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportStartedFile)
		, audioStandaloneFile(pACMRData->audioStandaloneFile)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;

	CATLStandaloneFile& audioStandaloneFile;
	bool const          bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> final : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(CATLStandaloneFile& _audioStandaloneFile)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportStoppedFile)
		, audioStandaloneFile(_audioStandaloneFile)
	{}

	explicit SAudioCallbackManagerRequestData(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> const* const pACMRData)
		: SAudioCallbackManagerRequestDataBase(eAudioCallbackManagerRequestType_ReportStoppedFile)
		, audioStandaloneFile(pACMRData->audioStandaloneFile)
	{}

	virtual ~SAudioCallbackManagerRequestData() override = default;

	CATLStandaloneFile& audioStandaloneFile;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioObjectRequestDataBase : public SAudioRequestData
{
	explicit SAudioObjectRequestDataBase(EAudioObjectRequestType const _type)
		: SAudioRequestData(eAudioRequestType_AudioObjectRequest)
		, type(_type)
	{}

	virtual ~SAudioObjectRequestDataBase() override = default;

	EAudioObjectRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioObjectRequestType T>
struct SAudioObjectRequestData final : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(T)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<T> const* const pAORData)
		: SAudioObjectRequestDataBase(T)
	{}

	virtual ~SAudioObjectRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_LoadTrigger> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(ControlId const _audioTriggerId)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_LoadTrigger)
		, audioTriggerId(_audioTriggerId)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_LoadTrigger> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_LoadTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_UnloadTrigger> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(ControlId const _audioTriggerId)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_UnloadTrigger)
		, audioTriggerId(_audioTriggerId)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_UnloadTrigger> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_UnloadTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(
	  CryFixedStringT<MaxFilePathLength> const& _file,
	  ControlId const _usedAudioTriggerId,
	  bool const _bLocalized)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_PlayFile)
		, file(_file)
		, usedAudioTriggerId(_usedAudioTriggerId)
		, bLocalized(_bLocalized)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_PlayFile)
		, file(pAORData->file)
		, usedAudioTriggerId(pAORData->usedAudioTriggerId)
		, bLocalized(pAORData->bLocalized)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
	ControlId const                          usedAudioTriggerId;
	bool const                               bLocalized;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_StopFile> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(CryFixedStringT<MaxFilePathLength> const& _file)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_StopFile)
		, file(_file)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_StopFile> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_StopFile)
		, file(pAORData->file)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(ControlId const _audioTriggerId)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_ExecuteTrigger)
		, audioTriggerId(_audioTriggerId)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_ExecuteTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTriggerEx> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(SExecuteTriggerData const& _data)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_ExecuteTriggerEx)
		, data(_data)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTriggerEx> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_ExecuteTriggerEx)
		, data(pAORData->data)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	SExecuteTriggerData const data;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(ControlId const _audioTriggerId)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_StopTrigger)
		, audioTriggerId(_audioTriggerId)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_StopTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	ControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_SetTransformation> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(CObjectTransformation const& _transformation)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetTransformation)
		, transformation(_transformation)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_SetTransformation> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetTransformation)
		, transformation(pAORData->transformation)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	CObjectTransformation const transformation;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(ControlId const _parameterId, float const _value)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetParameter)
		, parameterId(_parameterId)
		, value(_value)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetParameter)
		, parameterId(pAORData->parameterId)
		, value(pAORData->value)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	ControlId const parameterId;
	float const     value;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(ControlId const _audioSwitchId, SwitchStateId const _audioSwitchStateId)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetSwitchState)
		, audioSwitchId(_audioSwitchId)
		, audioSwitchStateId(_audioSwitchStateId)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetSwitchState)
		, audioSwitchId(pAORData->audioSwitchId)
		, audioSwitchStateId(pAORData->audioSwitchStateId)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	ControlId const     audioSwitchId;
	SwitchStateId const audioSwitchStateId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_SetCurrentEnvironments> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(EntityId const _entityToIgnore, Vec3 const& _position)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetCurrentEnvironments)
		, entityToIgnore(_entityToIgnore)
		, position(_position)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_SetCurrentEnvironments> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetCurrentEnvironments)
		, entityToIgnore(pAORData->entityToIgnore)
		, position(pAORData->position)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	EntityId const entityToIgnore;
	Vec3 const     position;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_SetEnvironment> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(EnvironmentId const _audioEnvironmentId, float const _amount)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetEnvironment)
		, audioEnvironmentId(_audioEnvironmentId)
		, amount(_amount)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_SetEnvironment> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_SetEnvironment)
		, audioEnvironmentId(pAORData->audioEnvironmentId)
		, amount(pAORData->amount)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	EnvironmentId const audioEnvironmentId;
	float const         amount;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAudioObjectRequestType_ProcessPhysicsRay> final : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(CAudioRayInfo* const _pAudioRayInfo)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_ProcessPhysicsRay)
		, pAudioRayInfo(_pAudioRayInfo)
	{}

	explicit SAudioObjectRequestData(SAudioObjectRequestData<eAudioObjectRequestType_ProcessPhysicsRay> const* const pAORData)
		: SAudioObjectRequestDataBase(eAudioObjectRequestType_ProcessPhysicsRay)
		, pAudioRayInfo(pAORData->pAudioRayInfo)
	{}

	virtual ~SAudioObjectRequestData() override = default;

	CAudioRayInfo* const pAudioRayInfo;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioListenerRequestDataBase : public SAudioRequestData
{
	explicit SAudioListenerRequestDataBase(EAudioListenerRequestType const _type)
		: SAudioRequestData(eAudioRequestType_AudioListenerRequest)
		, type(_type)
	{}

	virtual ~SAudioListenerRequestDataBase() override = default;

	EAudioListenerRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioListenerRequestType T>
struct SAudioListenerRequestData final : public SAudioListenerRequestDataBase
{
	SAudioListenerRequestData()
		: SAudioListenerRequestDataBase(T)
	{}

	explicit SAudioListenerRequestData(SAudioListenerRequestData<T> const* const pALRData)
		: SAudioListenerRequestDataBase(T)
	{}

	virtual ~SAudioListenerRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioListenerRequestData<eAudioListenerRequestType_SetTransformation> final : public SAudioListenerRequestDataBase
{
	explicit SAudioListenerRequestData(
	  CObjectTransformation const& _transformation,
	  CATLListener* const _pListener)
		: SAudioListenerRequestDataBase(eAudioListenerRequestType_SetTransformation)
		, transformation(_transformation)
		, pListener(_pListener)
	{}

	explicit SAudioListenerRequestData(SAudioListenerRequestData<eAudioListenerRequestType_SetTransformation> const* const pALRData)
		: SAudioListenerRequestDataBase(eAudioListenerRequestType_SetTransformation)
		, transformation(pALRData->transformation)
		, pListener(pALRData->pListener)
	{}

	virtual ~SAudioListenerRequestData() override = default;

	CObjectTransformation const transformation;
	CATLListener* const         pListener;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioListenerRequestData<eAudioListenerRequestType_ReleaseListener> final : public SAudioListenerRequestDataBase
{
	explicit SAudioListenerRequestData(CATLListener* const _pListener)
		: SAudioListenerRequestDataBase(eAudioListenerRequestType_ReleaseListener)
		, pListener(_pListener)
	{}

	explicit SAudioListenerRequestData(SAudioListenerRequestData<eAudioListenerRequestType_ReleaseListener> const* const pALRData)
		: SAudioListenerRequestDataBase(eAudioListenerRequestType_ReleaseListener)
		, pListener(pALRData->pListener)
	{}

	virtual ~SAudioListenerRequestData() override = default;

	CATLListener* const pListener;
};

SAudioRequestData* AllocateRequestData(SAudioRequestData const* const pRequestData);

class CAudioRequest
{
public:

	CAudioRequest() = default;

	explicit CAudioRequest(SAudioRequestData const* const pRequestData)
		: flags(eRequestFlags_None)
		, pObject(nullptr)
		, pOwner(nullptr)
		, pUserData(nullptr)
		, pUserDataOwner(nullptr)
		, status(eRequestStatus_None)
		, infoFlags(eAudioRequestInfoFlags_None)
		, pData(AllocateRequestData(pRequestData))
	{}

	explicit CAudioRequest(
	  EnumFlagsType const _flags,
	  CATLAudioObject* const _pObject,
	  void* const _pOwner,
	  void* const _pUserData,
	  void* const _pUserDataOwner,
	  SAudioRequestData const* const _pRequestData)
		: flags(_flags)
		, pObject(_pObject)
		, pOwner(_pOwner)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
		, status(eRequestStatus_None)
		, infoFlags(eAudioRequestInfoFlags_None)
		, pData(AllocateRequestData(_pRequestData))
	{}

	_smart_ptr<SAudioRequestData> GetData() const { return pData; }

	EnumFlagsType    flags = eRequestFlags_None;
	CATLAudioObject* pObject = nullptr;
	void*            pOwner = nullptr;
	void*            pUserData = nullptr;
	void*            pUserDataOwner = nullptr;
	ERequestStatus   status = eRequestStatus_None;
	EnumFlagsType    infoFlags = eAudioRequestInfoFlags_None;

private:

	// Must be private as it needs "AllocateRequestData"!
	_smart_ptr<SAudioRequestData> pData = nullptr;
};

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
// Filter for drawing debug info to the screen
enum EAudioDebugDrawFilter : EnumFlagsType
{
	eADDF_ALL                          = 0,
	eADDF_DRAW_SPHERES                 = BIT(6),    // a
	eADDF_SHOW_OBJECT_LABEL            = BIT(7),    // b
	eADDF_SHOW_OBJECT_TRIGGERS         = BIT(8),    // c
	eADDF_SHOW_OBJECT_STATES           = BIT(9),    // d
	eADDF_SHOW_OBJECT_PARAMETERS       = BIT(10),   // e
	eADDF_SHOW_OBJECT_ENVIRONMENTS     = BIT(11),   // f
	eADDF_DRAW_OBSTRUCTION_RAYS        = BIT(12),   // g
	eADDF_SHOW_OBSTRUCTION_RAY_LABELS  = BIT(13),   // h
	eADDF_DRAW_OBJECT_STANDALONE_FILES = BIT(14),   // i

	eADDF_SHOW_STANDALONE_FILES        = BIT(26),   // u
	eADDF_SHOW_ACTIVE_EVENTS           = BIT(27),   // v
	eADDF_SHOW_ACTIVE_OBJECTS          = BIT(28),   // w
	eADDF_SHOW_FILECACHE_MANAGER_INFO  = BIT(29),   // x
};
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

} // namespace CryAudio
