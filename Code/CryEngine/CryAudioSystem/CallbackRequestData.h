// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
	ReportPhysicalizedObject,
	ReportVirtualizedObject,
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
struct SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(TriggerInstanceId const triggerInstanceId_, ETriggerResult const result_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedTriggerConnectionInstance)
		, triggerInstanceId(triggerInstanceId_)
		, result(result_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportStartedTriggerConnectionInstance> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportStartedTriggerConnectionInstance)
		, triggerInstanceId(pACMRData->triggerInstanceId)
		, result(pACMRData->result)
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

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerConnectionInstance> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportFinishedTriggerConnectionInstance)
		, triggerInstanceId(pACMRData->triggerInstanceId)
		, result(pACMRData->result)
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
struct SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject> final : public SCallbackRequestDataBase, public CPoolObject<SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject>, stl::PSyncMultiThread>
{
	explicit SCallbackRequestData(Impl::IObject* const pIObject_)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportPhysicalizedObject)
		, pIObject(pIObject_)
	{}

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportPhysicalizedObject> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportPhysicalizedObject)
		, pIObject(pACMRData->pIObject)
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

	explicit SCallbackRequestData(SCallbackRequestData<ECallbackRequestType::ReportVirtualizedObject> const* const pACMRData)
		: SCallbackRequestDataBase(ECallbackRequestType::ReportVirtualizedObject)
		, pIObject(pACMRData->pIObject)
	{}

	virtual ~SCallbackRequestData() override = default;

	Impl::IObject* const pIObject;
};
} // namespace CryAudio
