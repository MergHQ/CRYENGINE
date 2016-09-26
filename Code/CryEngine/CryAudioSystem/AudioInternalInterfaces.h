// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>
#include <IAudioImpl.h>
#include <CryString/CryName.h>

enum EAudioRequestInfoFlags : AudioEnumFlagsType
{
	eAudioRequestInfoFlags_None              = 0,
	eAudioRequestInfoFlags_WaitingForRemoval = BIT(0),
};

enum EAudioStandaloneFileState : AudioEnumFlagsType
{
	eAudioStandaloneFileState_None,
	eAudioStandaloneFileState_Playing,
	eAudioStandaloneFileState_Stopping,
	eAudioStandaloneFileState_Loading,
};

//////////////////////////////////////////////////////////////////////////
struct SAudioEventListener
{
	SAudioEventListener()
		: pObjectToListenTo(nullptr)
		, OnEvent(nullptr)
		, requestType(eAudioRequestType_AudioAllRequests)
		, specificRequestMask(ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS)
	{}

	void const*        pObjectToListenTo;
	void               (* OnEvent)(SAudioRequestInfo const* const);

	EAudioRequestType  requestType;
	AudioEnumFlagsType specificRequestMask;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioRequestDataInternal : public _i_multithread_reference_target_t
{
	explicit SAudioRequestDataInternal(EAudioRequestType const _type)
		: type(_type)
	{}

	virtual ~SAudioRequestDataInternal() override = default;

	SAudioRequestDataInternal(SAudioRequestDataInternal const&) = delete;
	SAudioRequestDataInternal(SAudioRequestDataInternal&&) = delete;
	SAudioRequestDataInternal& operator=(SAudioRequestDataInternal const&) = delete;
	SAudioRequestDataInternal& operator=(SAudioRequestDataInternal&&) = delete;

	// _i_multithread_reference_target_t
	virtual void Release() override;
	// ~_i_multithread_reference_target_t

	EAudioRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioManagerRequestDataInternalBase : public SAudioRequestDataInternal
{
	explicit SAudioManagerRequestDataInternalBase(EAudioManagerRequestType const _type)
		: SAudioRequestDataInternal(eAudioRequestType_AudioManagerRequest)
		, type(_type)
	{}

	virtual ~SAudioManagerRequestDataInternalBase() override = default;

	EAudioManagerRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioManagerRequestType T>
struct SAudioManagerRequestDataInternal final : public SAudioManagerRequestDataInternalBase
{
	SAudioManagerRequestDataInternal()
		: SAudioManagerRequestDataInternalBase(T)
	{}

	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestDataInternal<T> const* const pAMRDataInternal)
		: SAudioManagerRequestDataInternalBase(T)
	{}

	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<T> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(T)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_SetAudioImpl> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_SetAudioImpl> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_SetAudioImpl)
		, pImpl(pAMRData->pImpl)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	CryAudio::Impl::IAudioImpl* const pImpl;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_ReserveAudioObjectId> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_ReserveAudioObjectId> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_ReserveAudioObjectId)
		, pAudioObjectId(pAMRData->pAudioObjectId)
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		, audioObjectName(pAMRData->szAudioObjectName)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	AudioObjectId* const pAudioObjectId;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_OBJECT_NAME_LENGTH> audioObjectName;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_AddRequestListener> final : public SAudioManagerRequestDataInternalBase
{
	// Used when constructed internally.
	explicit SAudioManagerRequestDataInternal(
	  void const* const _pObjectToListenTo,
	  void(*_func)(SAudioRequestInfo const* const),
	  EAudioRequestType const _requestType,
	  AudioEnumFlagsType const _specificRequestMask)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_AddRequestListener)
		, pObjectToListenTo(_pObjectToListenTo)
		, func(_func)
		, requestType(_requestType)
		, specificRequestMask(_specificRequestMask)
	{}

	// Used when allocated internally.
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestDataInternal<eAudioManagerRequestType_AddRequestListener> const* const pAMRDataInternal)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_AddRequestListener)
		, pObjectToListenTo(pAMRDataInternal->pObjectToListenTo)
		, func(pAMRDataInternal->func)
		, requestType(pAMRDataInternal->requestType)
		, specificRequestMask(pAMRDataInternal->specificRequestMask)
	{}

	// Used when copied from external to internal request.
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_AddRequestListener> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_AddRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
		, requestType(pAMRData->type)
		, specificRequestMask(pAMRData->specificRequestMask)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	void const* const        pObjectToListenTo;
	void                     (* func)(SAudioRequestInfo const* const);
	EAudioRequestType const  requestType;
	AudioEnumFlagsType const specificRequestMask;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_RemoveRequestListener> final : public SAudioManagerRequestDataInternalBase
{
	// Used when constructed internally.
	explicit SAudioManagerRequestDataInternal(void const* const _pObjectToListenTo, void(*_func)(SAudioRequestInfo const* const))
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_RemoveRequestListener)
		, pObjectToListenTo(_pObjectToListenTo)
		, func(_func)
	{}

	// Used when allocated internally.
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestDataInternal<eAudioManagerRequestType_RemoveRequestListener> const* const pAMRDataInternal)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_RemoveRequestListener)
		, pObjectToListenTo(pAMRDataInternal->pObjectToListenTo)
		, func(pAMRDataInternal->func)
	{}

	// Used when copied from external to internal request.
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_RemoveRequestListener> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_RemoveRequestListener)
		, pObjectToListenTo(pAMRData->pObjectToListenTo)
		, func(pAMRData->func)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	void const* const pObjectToListenTo;
	void              (* func)(SAudioRequestInfo const* const);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_ParseControlsData> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_ParseControlsData)
		, folderPath(pAMRData->szFolderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const folderPath;
	EAudioDataScope const                             dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_ParsePreloadsData> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_ParsePreloadsData)
		, folderPath(pAMRData->szFolderPath)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const folderPath;
	EAudioDataScope const                             dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_ClearControlsData> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_ClearControlsData> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_ClearControlsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	EAudioDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_ClearPreloadsData> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_ClearPreloadsData> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_ClearPreloadsData)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	EAudioDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_PreloadSingleRequest> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_PreloadSingleRequest)
		, audioPreloadRequestId(pAMRData->audioPreloadRequestId)
		, bAutoLoadOnly(pAMRData->bAutoLoadOnly)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	AudioPreloadRequestId const audioPreloadRequestId;
	bool const                  bAutoLoadOnly;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_UnloadSingleRequest> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_UnloadSingleRequest> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_UnloadSingleRequest)
		, audioPreloadRequestId(pAMRData->audioPreloadRequestId)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	AudioPreloadRequestId const audioPreloadRequestId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_UnloadAFCMDataByScope> final : public SAudioManagerRequestDataInternalBase
{
	// Used when constructed internally.
	explicit SAudioManagerRequestDataInternal(EAudioDataScope const _dataScope)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_UnloadAFCMDataByScope)
		, dataScope(_dataScope)
	{}

	// Used when allocated internally.
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestDataInternal<eAudioManagerRequestType_UnloadAFCMDataByScope> const* const pAMRDataInternal)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_UnloadAFCMDataByScope)
		, dataScope(pAMRDataInternal->dataScope)
	{}

	// Used when copied from external to internal request.
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_UnloadAFCMDataByScope> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_UnloadAFCMDataByScope)
		, dataScope(pAMRData->dataScope)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	EAudioDataScope const dataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_RefreshAudioSystem> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_RefreshAudioSystem> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_RefreshAudioSystem)
		, levelName(pAMRData->szLevelName)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_ReloadControlsData> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestData<eAudioManagerRequestType_ReloadControlsData> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_ReloadControlsData)
		, folderPath(pAMRData->szFolderPath)
		, levelName(pAMRData->szLevelName)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const folderPath;
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const levelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestDataInternal<eAudioManagerRequestType_GetAudioFileData> final : public SAudioManagerRequestDataInternalBase
{
	explicit SAudioManagerRequestDataInternal(SAudioManagerRequestDataInternal<eAudioManagerRequestType_GetAudioFileData> const* const pAMRData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_GetAudioFileData)
		, szFilename(pAMRData->szFilename)
		, audioFileData(pAMRData->audioFileData)
	{}

	explicit SAudioManagerRequestDataInternal(char const* const _szFilename, SAudioFileData& _audioFileData)
		: SAudioManagerRequestDataInternalBase(eAudioManagerRequestType_GetAudioFileData)
		, szFilename(_szFilename)
		, audioFileData(_audioFileData)
	{}

	virtual ~SAudioManagerRequestDataInternal() override = default;

	char const* const szFilename;
	SAudioFileData&   audioFileData;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioCallbackManagerRequestDataInternalBase : public SAudioRequestDataInternal
{
	explicit SAudioCallbackManagerRequestDataInternalBase(EAudioCallbackManagerRequestType const _type)
		: SAudioRequestDataInternal(eAudioRequestType_AudioCallbackManagerRequest)
		, type(_type)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternalBase() override = default;

	EAudioCallbackManagerRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioCallbackManagerRequestType T>
struct SAudioCallbackManagerRequestDataInternal final : public SAudioCallbackManagerRequestDataInternalBase
{
	SAudioCallbackManagerRequestDataInternal()
		: SAudioCallbackManagerRequestDataInternalBase(T)
	{}

	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<T> const* const pAMRData)
		: SAudioCallbackManagerRequestDataInternalBase(T)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestDataInternal<eAudioCallbackManagerRequestType_ReportStartedEvent> final : public SAudioCallbackManagerRequestDataInternalBase
{
	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataInternalBase(eAudioCallbackManagerRequestType_ReportStartedEvent)
		, audioEventId(pACMRData->audioEventId)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;

	AudioEventId const audioEventId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestDataInternal<eAudioCallbackManagerRequestType_ReportFinishedEvent> final : public SAudioCallbackManagerRequestDataInternalBase
{
	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataInternalBase(eAudioCallbackManagerRequestType_ReportFinishedEvent)
		, audioEventId(pACMRData->audioEventId)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;

	AudioEventId const audioEventId;
	bool const         bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestDataInternal<eAudioCallbackManagerRequestType_ReportVirtualizedEvent> final : public SAudioCallbackManagerRequestDataInternalBase
{
	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportVirtualizedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataInternalBase(eAudioCallbackManagerRequestType_ReportVirtualizedEvent)
		, audioEventId(pACMRData->audioEventId)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;

	AudioEventId const audioEventId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestDataInternal<eAudioCallbackManagerRequestType_ReportPhysicalizedEvent> final : public SAudioCallbackManagerRequestDataInternalBase
{
	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportPhysicalizedEvent> const* const pACMRData)
		: SAudioCallbackManagerRequestDataInternalBase(eAudioCallbackManagerRequestType_ReportPhysicalizedEvent)
		, audioEventId(pACMRData->audioEventId)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;

	AudioEventId const audioEventId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestDataInternal<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> final : public SAudioCallbackManagerRequestDataInternalBase
{
	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> const* const pACMRData)
		: SAudioCallbackManagerRequestDataInternalBase(eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance)
		, audioTriggerId(pACMRData->audioTriggerId)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;

	AudioControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestDataInternal<eAudioCallbackManagerRequestType_ReportStartedFile> final : public SAudioCallbackManagerRequestDataInternalBase
{
	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> const* const pACMRData)
		: SAudioCallbackManagerRequestDataInternalBase(eAudioCallbackManagerRequestType_ReportStartedFile)
		, audioStandaloneFileInstanceId(pACMRData->audioStandaloneFileInstanceId)
		, file(pACMRData->szFile)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;

	AudioStandaloneFileId const                       audioStandaloneFileInstanceId;
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const file;
	bool const bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestDataInternal<eAudioCallbackManagerRequestType_ReportStoppedFile> final : public SAudioCallbackManagerRequestDataInternalBase
{
	explicit SAudioCallbackManagerRequestDataInternal(SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> const* const pACMRData)
		: SAudioCallbackManagerRequestDataInternalBase(eAudioCallbackManagerRequestType_ReportStoppedFile)
		, audioStandaloneFileInstanceId(pACMRData->audioStandaloneFileInstanceId)
		, file(pACMRData->szFile)
	{}

	virtual ~SAudioCallbackManagerRequestDataInternal() override = default;

	AudioStandaloneFileId const                       audioStandaloneFileInstanceId;
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const file;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioObjectRequestDataInternalBase : public SAudioRequestDataInternal
{
	explicit SAudioObjectRequestDataInternalBase(EAudioObjectRequestType const _type)
		: SAudioRequestDataInternal(eAudioRequestType_AudioObjectRequest)
		, type(_type)
	{}

	virtual ~SAudioObjectRequestDataInternalBase() override = default;

	EAudioObjectRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioObjectRequestType T>
struct SAudioObjectRequestDataInternal final : public SAudioObjectRequestDataInternalBase
{
	SAudioObjectRequestDataInternal()
		: SAudioObjectRequestDataInternalBase(T)
	{}

	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<T> const* const pAMRData)
		: SAudioObjectRequestDataInternalBase(T)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_PrepareTrigger> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_PrepareTrigger> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_PrepareTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	AudioControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_UnprepareTrigger> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_UnprepareTrigger> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_UnprepareTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	AudioControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_PlayFile> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_PlayFile)
		, file(pAORData->szFile)
		, usedAudioTriggerId(pAORData->usedAudioTriggerId)
		, bLocalized(pAORData->bLocalized)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const file;
	AudioControlId const                              usedAudioTriggerId;
	bool const bLocalized;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_StopFile> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_StopFile> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_StopFile)
		, file(pAORData->szFile)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> const file;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_ExecuteTrigger> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_ExecuteTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
		, timeUntilRemovalInMS(pAORData->timeUntilRemovalInMS)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	AudioControlId const audioTriggerId;
	float const          timeUntilRemovalInMS;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_StopTrigger> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_StopTrigger)
		, audioTriggerId(pAORData->audioTriggerId)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	AudioControlId const audioTriggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_SetTransformation> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_SetTransformation> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_SetTransformation)
		, transformation(pAORData->transformation)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	CAudioObjectTransformation const transformation;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_SetRtpcValue> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_SetRtpcValue)
		, audioRtpcId(pAORData->audioRtpcId)
		, value(pAORData->value)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	AudioControlId const audioRtpcId;
	float const          value;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_SetSwitchState> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_SetSwitchState)
		, audioSwitchId(pAORData->audioSwitchId)
		, audioSwitchStateId(pAORData->audioSwitchStateId)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	AudioControlId const     audioSwitchId;
	AudioSwitchStateId const audioSwitchStateId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_SetVolume> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_SetVolume> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_SetVolume)
		, volume(pAORData->volume)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	float const volume;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_SetEnvironmentAmount> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_SetEnvironmentAmount> const* const pAORData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_SetEnvironmentAmount)
		, audioEnvironmentId(pAORData->audioEnvironmentId)
		, amount(pAORData->amount)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	AudioEnvironmentId const audioEnvironmentId;
	float const              amount;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestDataInternal<eAudioObjectRequestType_ProcessPhysicsRay> final : public SAudioObjectRequestDataInternalBase
{
	explicit SAudioObjectRequestDataInternal(SAudioObjectRequestData<eAudioObjectRequestType_ProcessPhysicsRay> const* const pACMRData)
		: SAudioObjectRequestDataInternalBase(eAudioObjectRequestType_ProcessPhysicsRay)
		, pAudioRayInfo(pACMRData->pAudioRayInfo)
	{}

	virtual ~SAudioObjectRequestDataInternal() override = default;

	CAudioRayInfo* const pAudioRayInfo;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioListenerRequestDataInternalBase : public SAudioRequestDataInternal
{
	explicit SAudioListenerRequestDataInternalBase(EAudioListenerRequestType const _type)
		: SAudioRequestDataInternal(eAudioRequestType_AudioListenerRequest)
		, type(_type)
	{}

	virtual ~SAudioListenerRequestDataInternalBase() override = default;

	EAudioListenerRequestType const type;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioListenerRequestType T>
struct SAudioListenerRequestDataInternal final : public SAudioListenerRequestDataInternalBase
{
	SAudioListenerRequestDataInternal()
		: SAudioListenerRequestDataInternalBase(T)
	{}

	explicit SAudioListenerRequestDataInternal(SAudioListenerRequestData<T> const* const pAMRData)
		: SAudioListenerRequestDataInternalBase(T)
	{}

	virtual ~SAudioListenerRequestDataInternal() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioListenerRequestDataInternal<eAudioListenerRequestType_SetTransformation> final : public SAudioListenerRequestDataInternalBase
{
	explicit SAudioListenerRequestDataInternal(SAudioListenerRequestData<eAudioListenerRequestType_SetTransformation> const* const pALRData)
		: SAudioListenerRequestDataInternalBase(eAudioListenerRequestType_SetTransformation)
		, transformation(pALRData->transformation)
	{}

	virtual ~SAudioListenerRequestDataInternal() override = default;

	CAudioObjectTransformation const transformation;
};

SAudioRequestDataInternal* ConvertToInternal(SAudioRequestDataBase const* const pExternalData);
SAudioRequestDataInternal* AllocateForInternal(SAudioRequestDataInternal const* const pRequestDataInternal);

class CAudioRequestInternal
{
public:

	CAudioRequestInternal() = default;

	explicit CAudioRequestInternal(SAudioRequest const& externalRequest)
		: flags(externalRequest.flags)
		, audioObjectId(externalRequest.audioObjectId)
		, pOwner(externalRequest.pOwner)
		, pUserData(externalRequest.pUserData)
		, pUserDataOwner(externalRequest.pUserDataOwner)
		, status(eAudioRequestStatus_None)
		, internalInfoFlags(eAudioRequestInfoFlags_None)
		, pData(ConvertToInternal(externalRequest.pData))
	{}

	explicit CAudioRequestInternal(SAudioRequestDataInternal const* const pRequestDataInternal)
		: flags(eAudioRequestFlags_None)
		, audioObjectId(INVALID_AUDIO_OBJECT_ID)
		, pOwner(nullptr)
		, pUserData(nullptr)
		, pUserDataOwner(nullptr)
		, status(eAudioRequestStatus_None)
		, internalInfoFlags(eAudioRequestInfoFlags_None)
		, pData(AllocateForInternal(pRequestDataInternal))
	{}

	AudioEnumFlagsType                    flags = eAudioRequestFlags_None;
	AudioObjectId                         audioObjectId = INVALID_AUDIO_OBJECT_ID;
	void*                                 pOwner = nullptr;
	void*                                 pUserData = nullptr;
	void*                                 pUserDataOwner = nullptr;
	EAudioRequestStatus                   status = eAudioRequestStatus_None;
	AudioEnumFlagsType                    internalInfoFlags = eAudioRequestInfoFlags_None;
	_smart_ptr<SAudioRequestDataInternal> pData = nullptr;
};

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
// Filter for drawing debug info to the screen
enum EAudioDebugDrawFilter : AudioEnumFlagsType
{
	eADDF_ALL                          = 0,
	eADDF_DRAW_SPHERES                 = BIT(6),  // a
	eADDF_SHOW_OBJECT_LABEL            = BIT(7),  // b
	eADDF_SHOW_OBJECT_TRIGGERS         = BIT(8),  // c
	eADDF_SHOW_OBJECT_STATES           = BIT(9),  // d
	eADDF_SHOW_OBJECT_RTPCS            = BIT(10), // e
	eADDF_SHOW_OBJECT_ENVIRONMENTS     = BIT(11), // f
	eADDF_DRAW_OBSTRUCTION_RAYS        = BIT(12), // g
	eADDF_SHOW_OBSTRUCTION_RAY_LABELS  = BIT(13), // h
	eADDF_DRAW_OBJECT_STANDALONE_FILES = BIT(14), // i

	eADDF_SHOW_STANDALONE_FILES        = BIT(26), // u
	eADDF_SHOW_ACTIVE_EVENTS           = BIT(27), // v
	eADDF_SHOW_ACTIVE_OBJECTS          = BIT(28), // w
	eADDF_SHOW_FILECACHE_MANAGER_INFO  = BIT(29), // x
};
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
