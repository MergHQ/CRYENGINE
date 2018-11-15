// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include "Common/PoolObject.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CObject;
class CRayInfo;

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

//////////////////////////////////////////////////////////////////////////
struct SObjectRequestDataBase : public SRequestData
{
	explicit SObjectRequestDataBase(EObjectRequestType const objectRequestType_, CObject* const pObject_)
		: SRequestData(ERequestType::ObjectRequest)
		, objectRequestType(objectRequestType_)
		, pObject(pObject_)
	{}

	virtual ~SObjectRequestDataBase() override = default;

	EObjectRequestType const objectRequestType;
	CObject* const           pObject;
};

//////////////////////////////////////////////////////////////////////////
template<EObjectRequestType T>
struct SObjectRequestData final : public SObjectRequestDataBase
{
	SObjectRequestData(CObject* const pObject_)
		: SObjectRequestDataBase(T, pObject_)
	{}

	explicit SObjectRequestData(SObjectRequestData<T> const* const pORData)
		: SObjectRequestDataBase(T, pORData->pObject)
	{}

	virtual ~SObjectRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::LoadTrigger> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::LoadTrigger, pObject_)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::LoadTrigger> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::LoadTrigger, pORData->pObject)
		, triggerId(pORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::UnloadTrigger> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::UnloadTrigger, pObject_)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::UnloadTrigger> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::UnloadTrigger, pORData->pObject)
		, triggerId(pORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::PlayFile> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(
		CObject* const pObject_,
		CryFixedStringT<MaxFilePathLength> const& file_,
		ControlId const usedTriggerId_,
		bool const bLocalized_)
		: SObjectRequestDataBase(EObjectRequestType::PlayFile, pObject_)
		, file(file_)
		, usedTriggerId(usedTriggerId_)
		, bLocalized(bLocalized_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::PlayFile> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::PlayFile, pORData->pObject)
		, file(pORData->file)
		, usedTriggerId(pORData->usedTriggerId)
		, bLocalized(pORData->bLocalized)
	{}

	virtual ~SObjectRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
	ControlId const                          usedTriggerId;
	bool const                               bLocalized;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::StopFile> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, CryFixedStringT<MaxFilePathLength> const& file_)
		: SObjectRequestDataBase(EObjectRequestType::StopFile, pObject_)
		, file(file_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::StopFile> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::StopFile, pORData->pObject)
		, file(pORData->file)
	{}

	virtual ~SObjectRequestData() override = default;

	CryFixedStringT<MaxFilePathLength> const file;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ExecuteTrigger> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::ExecuteTrigger>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::ExecuteTrigger, pObject_)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ExecuteTrigger> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::ExecuteTrigger, pORData->pObject)
		, triggerId(pORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::StopTrigger> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::StopTrigger>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::StopTrigger, pObject_)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::StopTrigger> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::StopTrigger, pORData->pObject)
		, triggerId(pORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetTransformation> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetTransformation>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, CTransformation const& transformation_)
		: SObjectRequestDataBase(EObjectRequestType::SetTransformation, pObject_)
		, transformation(transformation_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetTransformation> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetTransformation, pORData->pObject)
		, transformation(pORData->transformation)
	{}

	virtual ~SObjectRequestData() override = default;

	CTransformation const transformation;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetOcclusionType> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, EOcclusionType const occlusionType_)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionType, pObject_)
		, occlusionType(occlusionType_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetOcclusionType> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionType, pORData->pObject)
		, occlusionType(pORData->occlusionType)
	{}

	virtual ~SObjectRequestData() override = default;

	EOcclusionType const occlusionType;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, float const occlusionRayOffset_)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionRayOffset, pObject_)
		, occlusionRayOffset(occlusionRayOffset_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetOcclusionRayOffset> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetOcclusionRayOffset, pORData->pObject)
		, occlusionRayOffset(pORData->occlusionRayOffset)
	{}

	virtual ~SObjectRequestData() override = default;

	float const occlusionRayOffset;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetParameter> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetParameter>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, ControlId const parameterId_, float const value_)
		: SObjectRequestDataBase(EObjectRequestType::SetParameter, pObject_)
		, parameterId(parameterId_)
		, value(value_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetParameter> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetParameter, pORData->pObject)
		, parameterId(pORData->parameterId)
		, value(pORData->value)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const parameterId;
	float const     value;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetSwitchState> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetSwitchState>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, ControlId const switchId_, SwitchStateId const switchStateId_)
		: SObjectRequestDataBase(EObjectRequestType::SetSwitchState, pObject_)
		, switchId(switchId_)
		, switchStateId(switchStateId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetSwitchState> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetSwitchState, pORData->pObject)
		, switchId(pORData->switchId)
		, switchStateId(pORData->switchStateId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const     switchId;
	SwitchStateId const switchStateId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetCurrentEnvironments>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, EntityId const entityToIgnore_)
		: SObjectRequestDataBase(EObjectRequestType::SetCurrentEnvironments, pObject_)
		, entityToIgnore(entityToIgnore_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetCurrentEnvironments, pORData->pObject)
		, entityToIgnore(pORData->entityToIgnore)
	{}

	virtual ~SObjectRequestData() override = default;

	EntityId const entityToIgnore;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetEnvironment> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetEnvironment>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, EnvironmentId const environmentId_, float const amount_)
		: SObjectRequestDataBase(EObjectRequestType::SetEnvironment, pObject_)
		, environmentId(environmentId_)
		, amount(amount_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetEnvironment> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetEnvironment, pORData->pObject)
		, environmentId(pORData->environmentId)
		, amount(pORData->amount)
	{}

	virtual ~SObjectRequestData() override = default;

	EnvironmentId const environmentId;
	float const         amount;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::ProcessPhysicsRay>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CObject* const pObject_, CRayInfo* const pRayInfo_)
		: SObjectRequestDataBase(EObjectRequestType::ProcessPhysicsRay, pObject_)
		, pRayInfo(pRayInfo_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::ProcessPhysicsRay, pORData->pObject)
		, pRayInfo(pORData->pRayInfo)
	{}

	virtual ~SObjectRequestData() override = default;

	CRayInfo* const pRayInfo;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetName> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, char const* const szName)
		: SObjectRequestDataBase(EObjectRequestType::SetName, pObject_)
		, name(szName)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetName> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::SetName, pORData->pObject)
		, name(pORData->name)
	{}

	virtual ~SObjectRequestData() override = default;

	CryFixedStringT<MaxObjectNameLength> const name;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, bool const isEnabled_)
		: SObjectRequestDataBase(EObjectRequestType::ToggleAbsoluteVelocityTracking, pObject_)
		, isEnabled(isEnabled_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ToggleAbsoluteVelocityTracking> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::ToggleAbsoluteVelocityTracking, pORData->pObject)
		, isEnabled(pORData->isEnabled)
	{}

	virtual ~SObjectRequestData() override = default;

	bool const isEnabled;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(CObject* const pObject_, bool const isEnabled_)
		: SObjectRequestDataBase(EObjectRequestType::ToggleRelativeVelocityTracking, pObject_)
		, isEnabled(isEnabled_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ToggleRelativeVelocityTracking> const* const pORData)
		: SObjectRequestDataBase(EObjectRequestType::ToggleRelativeVelocityTracking, pORData->pObject)
		, isEnabled(pORData->isEnabled)
	{}

	virtual ~SObjectRequestData() override = default;

	bool const isEnabled;
};
} // namespace CryAudio
