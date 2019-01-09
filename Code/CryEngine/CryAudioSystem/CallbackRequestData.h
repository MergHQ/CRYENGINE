// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include "Common/PoolObject.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
enum class ECallbackRequestType : EnumFlagsType
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

//////////////////////////////////////////////////////////////////////////
struct SCallbackRequestDataBase : public SRequestData
{
	explicit SCallbackRequestDataBase(ECallbackRequestType const callbackRequestType_)
		: SRequestData(ERequestType::CallbackRequest)
		, callbackRequestType(callbackRequestType_)
	{}

	virtual ~SCallbackRequestDataBase() override = default;

	ECallbackRequestType const callbackRequestType;
};

//////////////////////////////////////////////////////////////////////////
template<ECallbackRequestType T>
struct SCallbackRequestData final : public SCallbackRequestDataBase
{
	SCallbackRequestData()
		: SCallbackRequestDataBase(T)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<T> const* const pACMRData)
		: SCallbackRequestDataBase(T)
	{}

	virtual ~SCallbackRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportStartedEvent> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportStartedEvent>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(CEvent& event_, bool const isVirtual_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedEvent)
		, event(event_)
		, isVirtual(isVirtual_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportStartedEvent> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedEvent)
		, event(pACMRData->event)
		, isVirtual(pACMRData->isVirtual)
	{}

	virtual ~SCallbackRequestData() override = default;

	CEvent&    event;
	bool const isVirtual;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportFinishedEvent> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportFinishedEvent>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(CEvent& event_, bool const bSuccess_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedEvent)
		, event(event_)
		, bSuccess(bSuccess_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportFinishedEvent> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedEvent)
		, event(pACMRData->event)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SCallbackRequestData() override = default;

	CEvent&    event;
	bool const bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportVirtualizedEvent> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportVirtualizedEvent>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(CEvent& event_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportVirtualizedEvent)
		, event(event_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportVirtualizedEvent> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportVirtualizedEvent)
		, event(pACMRData->event)
	{}

	virtual ~SCallbackRequestData() override = default;

	CEvent& event;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedEvent> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedEvent>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(CEvent& event_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportPhysicalizedEvent)
		, event(event_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedEvent> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportPhysicalizedEvent)
		, event(pACMRData->event)
	{}

	virtual ~SCallbackRequestData() override = default;

	CEvent& event;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance>, stl::PSyncNone> // Intentionally not PSyncMultiThread because this gets only called from the audio thread.
{
	explicit SCallbackRequestData(ControlId const triggerId_, EntityId const entityId_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedTriggerInstance)
		, triggerId(triggerId_)
		, entityId(entityId_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedTriggerInstance)
		, triggerId(pACMRData->triggerId)
		, entityId(pACMRData->entityId)
	{}

	virtual ~SCallbackRequestData() override = default;

	ControlId const triggerId;
	EntityId const  entityId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportStartedFile> final : public SCallbackRequestDataBase
{
	explicit SCallbackRequestData(CStandaloneFile& standaloneFile_, bool const bSuccess_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedFile)
		, standaloneFile(standaloneFile_)
		, bSuccess(bSuccess_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportStartedFile> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedFile)
		, standaloneFile(pACMRData->standaloneFile)
		, bSuccess(pACMRData->bSuccess)
	{}

	virtual ~SCallbackRequestData() override = default;

	CStandaloneFile& standaloneFile;
	bool const       bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportStoppedFile> final : public SCallbackRequestDataBase
{
	explicit SCallbackRequestData(CStandaloneFile& standaloneFile_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStoppedFile)
		, standaloneFile(standaloneFile_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportStoppedFile> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStoppedFile)
		, standaloneFile(pACMRData->standaloneFile)
	{}

	virtual ~SCallbackRequestData() override = default;

	CStandaloneFile& standaloneFile;
};
} // namespace CryAudio
