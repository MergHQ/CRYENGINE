// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PropagationProcessor.h"
#include "Common/PoolObject.h"
#include <CryAudio/IObject.h>
#include <CrySystem/TimeValue.h>

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
struct IRenderAuxGeom;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
class CSystem;
class CFileManager;
class CEnvironment;
class CTrigger;
class CRequest;
class CStandaloneFile;
struct SRequestData;

namespace Impl
{
struct IObject;
} // namespace Impl

enum class EObjectFlags : EnumFlagsType
{
	None                  = 0,
	InUse                 = BIT(0),
	Active                = BIT(1),
	Virtual               = BIT(2),
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	CanRunOcclusion       = BIT(3),
	TrackAbsoluteVelocity = BIT(4),
	TrackRelativeVelocity = BIT(5),
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EObjectFlags);

enum class ETriggerStatus : EnumFlagsType
{
	None                     = 0,
	Playing                  = BIT(0),
	CallbackOnExternalThread = BIT(1),
	CallbackOnAudioThread    = BIT(2),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ETriggerStatus);

struct SUserDataBase
{
	SUserDataBase() = default;

	explicit SUserDataBase(
		void* const pOwnerOverride_,
		void* const pUserData_,
		void* const pUserDataOwner_)
		: pOwnerOverride(pOwnerOverride_)
		, pUserData(pUserData_)
		, pUserDataOwner(pUserDataOwner_)
	{}

	void* pOwnerOverride = nullptr;
	void* pUserData = nullptr;
	void* pUserDataOwner = nullptr;
};

struct STriggerInstanceState final : public SUserDataBase
{
	ETriggerStatus flags = ETriggerStatus::None;
	ControlId      triggerId = InvalidControlId;
	size_t         numPlayingInstances = 0;
	float          expirationTimeMS = 0.0f;
	float          remainingTimeMS = 0.0f;
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	float          radius = 0.0f;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};

// CObject-related typedefs
using ObjectStandaloneFileMap = std::map<CStandaloneFile*, SUserDataBase>;
using ObjectTriggerStates = std::map<TriggerInstanceId, STriggerInstanceState>;

class CObject final : public IObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	explicit CObject(CTransformation const& transformation, char const* const szName)
		: m_pImplData(nullptr)
		, m_transformation(transformation)
		, m_flags(EObjectFlags::InUse)
	#if defined(CRY_AUDIO_USE_OCCLUSION)
		, m_propagationProcessor(*this)
	#endif // CRY_AUDIO_USE_OCCLUSION
		, m_entityId(INVALID_ENTITYID)
		, m_numPendingSyncCallbacks(0)
		, m_maxRadius(0.0f)
		, m_name(szName)
	{}
#else
	explicit CObject(CTransformation const& transformation)
		: m_pImplData(nullptr)
		, m_transformation(transformation)
		, m_flags(EObjectFlags::InUse)
	#if defined(CRY_AUDIO_USE_OCCLUSION)
		, m_propagationProcessor(*this)
	#endif // CRY_AUDIO_USE_OCCLUSION
		, m_entityId(INVALID_ENTITYID)
		, m_numPendingSyncCallbacks(0)
	{}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	CObject() = delete;
	CObject(CObject const&) = delete;
	CObject(CObject&&) = delete;
	CObject& operator=(CObject const&) = delete;
	CObject& operator=(CObject&&) = delete;

	// CryAudio::IObject
	virtual EntityId GetEntityId() const override { return m_entityId; }
	// ~CryAudio::IObject

	ERequestStatus HandleStopTrigger(CTrigger const* const pTrigger);
	void           HandleSetTransformation(CTransformation const& transformation);
	void           HandleStopFile(char const* const szFile);

	void           Init(Impl::IObject* const pImplData, EntityId const entityId);
	void           Release();
	void           Destruct();

	// Callbacks
	void ReportFinishedStandaloneFile(CStandaloneFile* const pStandaloneFile);
	void GetStartedStandaloneFileRequestData(CStandaloneFile* const pStandaloneFile, CRequest& request);

	void StopAllTriggers();

#if defined(CRY_AUDIO_USE_OCCLUSION)
	void SetOcclusion(float const occlusion);
	void HandleSetOcclusionType(EOcclusionType const calcType);
	void ProcessPhysicsRay(CRayInfo& rayInfo);
	void HandleSetOcclusionRayOffset(float const offset);
	void ReleasePendingRays();
#endif // CRY_AUDIO_USE_OCCLUSION

	ObjectStandaloneFileMap const& GetActiveStandaloneFiles() const               { return m_activeStandaloneFiles; }

	void                           SetImplDataPtr(Impl::IObject* const pImplData) { m_pImplData = pImplData; }
	Impl::IObject*                 GetImplDataPtr() const                         { return m_pImplData; }

	CTransformation const&         GetTransformation() const                      { return m_transformation; }

	bool                           IsPlaying() const;
	bool                           HasPendingCallbacks() const;

	// Flags / Properties
	EObjectFlags GetFlags() const { return m_flags; }
	void         SetFlag(EObjectFlags const flag);
	void         RemoveFlag(EObjectFlags const flag);

	void         Update(float const deltaTime);

	void         IncrementSyncCallbackCounter() { CryInterlockedIncrement(&m_numPendingSyncCallbacks); }
	void         DecrementSyncCallbackCounter() { CRY_ASSERT(m_numPendingSyncCallbacks >= 1); CryInterlockedDecrement(&m_numPendingSyncCallbacks); }

	void         AddTriggerState(TriggerInstanceId const id, STriggerInstanceState const& triggerInstanceState);
	void         AddStandaloneFile(CStandaloneFile* const pStandaloneFile, SUserDataBase const& userDataBase);
	void         ReportFinishedTriggerInstance(TriggerInstanceId const triggerInstanceId);
	void         SendFinishedTriggerInstanceRequest(STriggerInstanceState const& triggerInstanceState);

private:

	// CryAudio::IObject
	virtual void ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void StopTrigger(ControlId const triggerId = InvalidControlId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetTransformation(CTransformation const& transformation, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetSwitchState(ControlId const switchId, SwitchStateId const switchStateId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetEnvironment(EnvironmentId const environmentId, float const amount, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetCurrentEnvironments(EntityId const entityToIgnore = 0, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetOcclusionRayOffset(float const offset, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void StopFile(char const* const szFile, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetName(char const* const szName, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	void         ToggleAbsoluteVelocityTracking(bool const enable, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	void         ToggleRelativeVelocityTracking(bool const enable, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	// ~CryAudio::IObject

	void PushRequest(SRequestData const& requestData, SRequestUserData const& userData);
	bool ExecuteDefaultTrigger(ControlId const id);

	ObjectStandaloneFileMap m_activeStandaloneFiles;
	ObjectTriggerStates     m_triggerStates;
	Impl::IObject*          m_pImplData;
	EObjectFlags            m_flags;
	CTransformation         m_transformation;
	EntityId                m_entityId;
	volatile int            m_numPendingSyncCallbacks;

#if defined(CRY_AUDIO_USE_OCCLUSION)
	CPropagationProcessor m_propagationProcessor;
#endif // CRY_AUDIO_USE_OCCLUSION

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
public:

	void           DrawDebugInfo(IRenderAuxGeom& auxGeom);
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	void           ResetObstructionRays() { m_propagationProcessor.ResetRayData(); }
	#endif // CRY_AUDIO_USE_OCCLUSION
	void           ForceImplementationRefresh(bool const setTransformation);

	char const*    GetName() const { return m_name.c_str(); }
	ERequestStatus HandleSetName(char const* const szName);
	void           StoreParameterValue(ControlId const id, float const value);
	void           StoreSwitchValue(ControlId const switchId, SwitchStateId const switchStateId);
	void           StoreEnvironmentValue(ControlId const id, float const value);

	void           UpdateMaxRadius(float const radius);

private:

	class CStateDebugDrawData final
	{
	public:

		CStateDebugDrawData(SwitchStateId const switchStateId);

		CStateDebugDrawData(CStateDebugDrawData const&) = delete;
		CStateDebugDrawData(CStateDebugDrawData&&) = delete;
		CStateDebugDrawData& operator=(CStateDebugDrawData const&) = delete;
		CStateDebugDrawData& operator=(CStateDebugDrawData&&) = delete;

		void                 Update(SwitchStateId const switchStateId);

		SwitchStateId m_currentStateId;
		float         m_currentSwitchColor;
	};

	using StateDrawInfoMap = std::map<ControlId, CStateDebugDrawData>;
	mutable StateDrawInfoMap m_stateDrawInfoMap;

	using SwitchStates = std::map<ControlId, SwitchStateId>;
	using Parameters = std::map<ControlId, float>;
	using Environments = std::map<EnvironmentId, float>;

	Parameters                           m_parameters;
	SwitchStates                         m_switchStates;
	Environments                         m_environments;
	float                                m_maxRadius;
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};
} // namespace CryAudio
