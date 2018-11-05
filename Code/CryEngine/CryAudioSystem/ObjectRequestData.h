// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include "Common/PoolObject.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
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
	explicit SObjectRequestData(ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::LoadTrigger)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::LoadTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::LoadTrigger)
		, triggerId(pAORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::UnloadTrigger> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::UnloadTrigger)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::UnloadTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::UnloadTrigger)
		, triggerId(pAORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::PlayFile> final : public SObjectRequestDataBase
{
	explicit SObjectRequestData(
		CryFixedStringT<MaxFilePathLength> const& file_,
		ControlId const usedTriggerId_,
		bool const bLocalized_)
		: SObjectRequestDataBase(EObjectRequestType::PlayFile)
		, file(file_)
		, usedTriggerId(usedTriggerId_)
		, bLocalized(bLocalized_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::PlayFile> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::PlayFile)
		, file(pAORData->file)
		, usedTriggerId(pAORData->usedTriggerId)
		, bLocalized(pAORData->bLocalized)
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
struct SObjectRequestData<EObjectRequestType::ExecuteTrigger> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::ExecuteTrigger>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::ExecuteTrigger)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ExecuteTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::ExecuteTrigger)
		, triggerId(pAORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::StopTrigger> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::StopTrigger>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(ControlId const triggerId_)
		: SObjectRequestDataBase(EObjectRequestType::StopTrigger)
		, triggerId(triggerId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::StopTrigger> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::StopTrigger)
		, triggerId(pAORData->triggerId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const triggerId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetTransformation> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetTransformation>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CTransformation const& transformation_)
		: SObjectRequestDataBase(EObjectRequestType::SetTransformation)
		, transformation(transformation_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetTransformation> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetTransformation)
		, transformation(pAORData->transformation)
	{}

	virtual ~SObjectRequestData() override = default;

	CTransformation const transformation;
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
struct SObjectRequestData<EObjectRequestType::SetParameter> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetParameter>, stl::PSyncMultiThread>
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
struct SObjectRequestData<EObjectRequestType::SetSwitchState> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetSwitchState>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(ControlId const switchId_, SwitchStateId const switchStateId_)
		: SObjectRequestDataBase(EObjectRequestType::SetSwitchState)
		, switchId(switchId_)
		, switchStateId(switchStateId_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetSwitchState> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetSwitchState)
		, switchId(pAORData->switchId)
		, switchStateId(pAORData->switchStateId)
	{}

	virtual ~SObjectRequestData() override = default;

	ControlId const     switchId;
	SwitchStateId const switchStateId;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::SetCurrentEnvironments> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetCurrentEnvironments>, stl::PSyncMultiThread>
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
struct SObjectRequestData<EObjectRequestType::SetEnvironment> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::SetEnvironment>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(EnvironmentId const environmentId_, float const amount_)
		: SObjectRequestDataBase(EObjectRequestType::SetEnvironment)
		, environmentId(environmentId_)
		, amount(amount_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::SetEnvironment> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::SetEnvironment)
		, environmentId(pAORData->environmentId)
		, amount(pAORData->amount)
	{}

	virtual ~SObjectRequestData() override = default;

	EnvironmentId const environmentId;
	float const         amount;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> final : public SObjectRequestDataBase, public CPoolObject<SObjectRequestData<EObjectRequestType::ProcessPhysicsRay>, stl::PSyncMultiThread>
{
	explicit SObjectRequestData(CRayInfo* const pRayInfo_)
		: SObjectRequestDataBase(EObjectRequestType::ProcessPhysicsRay)
		, pRayInfo(pRayInfo_)
	{}

	explicit SObjectRequestData(SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> const* const pAORData)
		: SObjectRequestDataBase(EObjectRequestType::ProcessPhysicsRay)
		, pRayInfo(pAORData->pRayInfo)
	{}

	virtual ~SObjectRequestData() override = default;

	CRayInfo* const pRayInfo;
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
} // namespace CryAudio
