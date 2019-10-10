// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include "Common/PoolObject.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
namespace Impl
{
struct IObject;
} // namespace Impl

enum class ECallbackRequestType : EnumFlagsType
{
	None,
	ReportStartedTriggerConnectionInstance,
	ReportFinishedTriggerConnectionInstance,
	ReportFinishedTriggerInstance,
	ReportTriggerConnectionInstanceCallback,
	SendTriggerInstanceCallback,
	ReportPhysicalizedObject,
	ReportVirtualizedObject,
	ReportContextActivated,
	ReportContextDeactivated,
	ReportFinishedPreload,
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

	explicit SCallbackRequestData(SCallbackRequestData<T> const* const pCRData)
		: SCallbackRequestDataBase(T)
	{}

	virtual ~SCallbackRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(TriggerInstanceId const triggerInstanceId_, ETriggerResult const result_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedTriggerConnectionInstance)
		, triggerInstanceId(triggerInstanceId_)
		, result(result_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedTriggerConnectionInstance)
		, triggerInstanceId(pCRData->triggerInstanceId)
		, result(pCRData->result)
	{}

	virtual ~SCallbackRequestData() override = default;

	TriggerInstanceId const triggerInstanceId;
	ETriggerResult const    result;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(TriggerInstanceId const triggerInstanceId_, ETriggerResult const result_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedTriggerConnectionInstance)
		, triggerInstanceId(triggerInstanceId_)
		, result(result_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedTriggerConnectionInstance)
		, triggerInstanceId(pCRData->triggerInstanceId)
		, result(pCRData->result)
	{}

	virtual ~SCallbackRequestData() override = default;

	TriggerInstanceId const triggerInstanceId;
	ETriggerResult const    result;
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

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedTriggerInstance)
		, triggerId(pCRData->triggerId)
		, entityId(pCRData->entityId)
	{}

	virtual ~SCallbackRequestData() override = default;

	ControlId const triggerId;
	EntityId const  entityId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportTriggerConnectionInstanceCallback> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportTriggerConnectionInstanceCallback>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(TriggerInstanceId const triggerInstanceId_, ESystemEvents const events_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportTriggerConnectionInstanceCallback)
		, triggerInstanceId(triggerInstanceId_)
		, events(events_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportTriggerConnectionInstanceCallback> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportTriggerConnectionInstanceCallback)
		, triggerInstanceId(pCRData->triggerInstanceId)
		, events(pCRData->events)
	{}

	virtual ~SCallbackRequestData() override = default;

	TriggerInstanceId const triggerInstanceId;
	ESystemEvents const     events;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::SendTriggerInstanceCallback> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::SendTriggerInstanceCallback>, stl::PSyncNone> // Intentionally not PSyncMultiThread because this gets only called from the audio thread.
{
	explicit SCallbackRequestData(ControlId const triggerId_, EntityId const entityId_, ESystemEvents const events_)
		: SCallbackRequestDataBase(ECallbackRequestType::SendTriggerInstanceCallback)
		, triggerId(triggerId_)
		, entityId(entityId_)
		, events(events_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::SendTriggerInstanceCallback> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::SendTriggerInstanceCallback)
		, triggerId(pCRData->triggerId)
		, entityId(pCRData->entityId)
		, events(pCRData->events)
	{}

	virtual ~SCallbackRequestData() override = default;

	ControlId const     triggerId;
	EntityId const      entityId;
	ESystemEvents const events;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(Impl::IObject* const pIObject_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportPhysicalizedObject)
		, pIObject(pIObject_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportPhysicalizedObject)
		, pIObject(pCRData->pIObject)
	{}

	virtual ~SCallbackRequestData() override = default;

	Impl::IObject* const pIObject;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(Impl::IObject* const pIObject_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportVirtualizedObject)
		, pIObject(pIObject_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportVirtualizedObject)
		, pIObject(pCRData->pIObject)
	{}

	virtual ~SCallbackRequestData() override = default;

	Impl::IObject* const pIObject;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SCallbackRequestData<ECallbackRequestType::ReportFinishedPreload> final : public SCallbackRequestDataBase
{
	explicit SCallbackRequestData(PreloadRequestId const preloadRequestId_, bool const isFullSuccess_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedPreload)
		, preloadRequestId(preloadRequestId_)
		, isFullSuccess(isFullSuccess_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportFinishedPreload> const* const pCRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedPreload)
		, preloadRequestId(pCRData->preloadRequestId)
		, isFullSuccess(pCRData->isFullSuccess)
	{}

	virtual ~SCallbackRequestData() override = default;

	PreloadRequestId const preloadRequestId;
	bool const             isFullSuccess;
};
} // namespace CryAudio
