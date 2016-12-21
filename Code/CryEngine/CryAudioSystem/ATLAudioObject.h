// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include "PropagationProcessor.h"
#include <CrySystem/TimeValue.h>
#include "PoolObject.h"
#include "SharedAudioData.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
struct IRenderAuxGeom;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
class CSystem;
class CAudioEventManager;
class CAudioListenerManager;
class CAudioStandaloneFileManager;

enum EAudioTriggerStatus : EnumFlagsType
{
	eAudioTriggerStatus_None                  = 0,
	eAudioTriggerStatus_Playing               = BIT(0),
	eAudioTriggerStatus_Loaded                = BIT(1),
	eAudioTriggerStatus_Loading               = BIT(2),
	eAudioTriggerStatus_Unloading             = BIT(3),
	eAudioTriggerStatus_Starting              = BIT(4),
	eAudioTriggerStatus_WaitingForRemoval     = BIT(5),
	eAudioTriggerStatus_CallbackOnAudioThread = BIT(6),
};

struct SAudioTriggerImplState
{
	EnumFlagsType flags = eAudioTriggerStatus_None;
};

struct SUserDataBase
{
	SUserDataBase() = default;

	explicit SUserDataBase(
	  void* const _pOwnerOverride,
	  void* const _pUserData,
	  void* const _pUserDataOwner)
		: pOwnerOverride(_pOwnerOverride)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
	{}

	void* pOwnerOverride = nullptr;
	void* pUserData = nullptr;
	void* pUserDataOwner = nullptr;
};

struct SAudioTriggerInstanceState final : public SUserDataBase
{
	EnumFlagsType flags = eAudioTriggerStatus_None;
	ControlId     audioTriggerId = InvalidControlId;
	size_t        numPlayingEvents = 0;
	size_t        numLoadingEvents = 0;
	float         expirationTimeMS = 0.0f;
	float         remainingTimeMS = 0.0f;
};

// CATLAudioObject-related typedefs
typedef std::map<CATLStandaloneFile*, SUserDataBase>            ObjectStandaloneFileMap;
typedef std::set<CATLEvent*>                                    ObjectEventSet;
typedef std::map<TriggerImplId, SAudioTriggerImplState>         ObjectTriggerImplStates;
typedef std::map<TriggerInstanceId, SAudioTriggerInstanceState> ObjectTriggerStates;
typedef std::map<ControlId, SwitchStateId>                      ObjectStateMap;
typedef std::map<ControlId, float>                              ObjectParameterMap;
typedef std::map<EnvironmentId, float>                          ObjectEnvironmentMap;

class CATLAudioObject final : public IObject, public CPoolObject<CATLAudioObject>
{
public:

	explicit CATLAudioObject(Impl::IAudioObject* const pImplData, Vec3 const& audioListenerPosition);

	CATLAudioObject(CATLAudioObject const&) = delete;
	CATLAudioObject(CATLAudioObject&&) = delete;
	CATLAudioObject& operator=(CATLAudioObject const&) = delete;
	CATLAudioObject& operator=(CATLAudioObject&&) = delete;

	// CryAudio::IObject
	virtual void ExecuteTrigger(ControlId const triggerId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void StopTrigger(ControlId const triggerId = InvalidControlId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetTransformation(CObjectTransformation const& transformation, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetParameter(ControlId const parameterId, float const value, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetSwitchState(ControlId const audioSwitchId, SwitchStateId const audioSwitchStateId, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetEnvironment(EnvironmentId const audioEnvironmentId, float const amount, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetCurrentEnvironments(EntityId const entityToIgnore = 0, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void ResetEnvironments(SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void PlayFile(SPlayFileInfo const& playFileInfo, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void StopFile(char const* const szFile, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	// ~CryAudio::IObject

	ERequestStatus HandleExecuteTrigger(CATLTrigger const* const pTrigger, void* const pOwner = nullptr, void* const pUserData = nullptr, void* const pUserDataOwner = nullptr, EnumFlagsType const flags = InvalidEnumFlagType);
	ERequestStatus HandleStopTrigger(CATLTrigger const* const pTrigger);
	ERequestStatus HandleSetTransformation(CObjectTransformation const& transformation, float const distanceToListener);
	ERequestStatus HandleSetParameter(CParameter const* const pParameter, float const value);
	ERequestStatus HandleSetSwitchState(CATLSwitch const* const pSwitch, CATLSwitchState const* const pState);
	ERequestStatus HandleSetEnvironment(CATLAudioEnvironment const* const pEnvironment, float const amount);
	ERequestStatus HandleResetEnvironments(AudioEnvironmentLookup const& environmentsLookup);
	void           HandleSetOcclusionType(EOcclusionType const calcType, Vec3 const& audioListenerPosition);
	ERequestStatus HandlePlayFile(CATLStandaloneFile* const pFile, void* const pOwner = nullptr, void* const pUserData = nullptr, void* const pUserDataOwner = nullptr);
	ERequestStatus HandleStopFile(char const* const szFile);

	void           Release();

	// Callbacks
	void                           ReportStartingTriggerInstance(TriggerInstanceId const audioTriggerInstanceId, ControlId const audioTriggerId);
	void                           ReportStartedTriggerInstance(TriggerInstanceId const audioTriggerInstanceId, void* const pOwnerOverride, void* const pUserData, void* const pUserDataOwner, EnumFlagsType const flags);
	void                           ReportStartedEvent(CATLEvent* const pEvent);
	void                           ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess);
	void                           ReportStartedStandaloneFile(CATLStandaloneFile* const pStandaloneFile, void* const pOwner, void* const pUserData, void* const pUserDataOwner);
	void                           ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile);
	void                           ReportFinishedLoadingTriggerImpl(TriggerImplId const audioTriggerImplId, bool const bLoad);
	void                           GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequest& request);

	ERequestStatus                 StopAllTriggers();
	ObjectEventSet const&          GetActiveEvents() const { return m_activeEvents; }
	ERequestStatus                 LoadTriggerAsync(CATLTrigger const* const pTrigger, bool const bLoad);

	float                          GetOcclusionFadeOutDistance() const { return m_occlusionFadeOutDistance; }
	void                           SetObstructionOcclusion(float const obstruction, float const occlusion);
	bool                           CanRunObstructionOcclusion() const  { return m_propagationProcessor.CanRunObstructionOcclusion(); }
	void                           ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo);
	bool                           HasNewOcclusionValues()             { return m_propagationProcessor.HasNewOcclusionValues(); }
	void                           GetPropagationData(SATLSoundPropagationData& propagationData) const;
	void                           ReleasePendingRays();

	ObjectStandaloneFileMap const& GetActiveStandaloneFiles() const                    { return m_activeStandaloneFiles; }

	void                           SetImplDataPtr(Impl::IAudioObject* const pImplData) { m_pImplData = pImplData; }
	Impl::IAudioObject*            GetImplDataPtr() const                              { return m_pImplData; }

	CObjectTransformation const&   GetTransformation()                                 { return m_attributes.transformation; }

	// Flags / Properties
	EnumFlagsType GetFlags() const { return m_flags; }
	void          SetFlag(EAudioObjectFlags const flag);
	void          RemoveFlag(EAudioObjectFlags const flag);
	void          SetDopplerTracking(bool const bEnable);
	void          SetVelocityTracking(bool const bEnable);
	float         GetMaxRadius() const { return m_maxRadius; }

	void          Update(float const deltaTime, float const distance, Vec3 const& audioListenerPosition);
	void          UpdateControls(float const deltaTime, Impl::SObject3DAttributes const& listenerAttributes);
	bool          CanBeReleased() const;

	static CSystem*                     s_pAudioSystem;
	static CAudioEventManager*          s_pEventManager;
	static CAudioStandaloneFileManager* s_pStandaloneFileManager;
	static ControlId                    s_occlusionTypeSwitchId;
	static SwitchStateId                s_occlusionTypeStateIds[eOcclusionType_Count];

private:

	void ReportFinishedTriggerInstance(ObjectTriggerStates::iterator& iter);
	void PushRequest(SAudioRequestData const& requestData, SRequestUserData const& userData);

	ObjectStandaloneFileMap   m_activeStandaloneFiles;
	ObjectEventSet            m_activeEvents;
	ObjectTriggerStates       m_triggerStates;
	ObjectTriggerImplStates   m_triggerImplStates;
	ObjectParameterMap        m_parameters;
	ObjectEnvironmentMap      m_environments;
	ObjectStateMap            m_switchStates;
	Impl::IAudioObject*       m_pImplData;
	float                     m_maxRadius = 0.0f;
	EnumFlagsType             m_flags = eAudioObjectFlags_None;
	float                     m_previousVelocity = 0.0f;
	Vec3                      m_velocity;
	Impl::SObject3DAttributes m_attributes;
	Impl::SObject3DAttributes m_previousAttributes;
	CTimeValue                m_previousTime;
	CPropagationProcessor     m_propagationProcessor;
	float                     m_occlusionFadeOutDistance = 0.0f;

	static TriggerInstanceId  s_triggerInstanceIdCounter;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(
	  IRenderAuxGeom& auxGeom,
	  Vec3 const& listenerPosition,
	  AudioTriggerLookup const& triggers,
	  AudioParameterLookup const& parameters,
	  AudioSwitchLookup const& switches,
	  AudioPreloadRequestLookup const& preloadRequests,
	  AudioEnvironmentLookup const& environments) const;
	void ResetObstructionRays() { m_propagationProcessor.ResetRayData(); }

	void ForceImplementationRefresh(
	  AudioTriggerLookup const& triggers,
	  AudioParameterLookup const& parameters,
	  AudioSwitchLookup const& switches,
	  AudioEnvironmentLookup const& environments,
	  bool const bSet3DAttributes);

	CryFixedStringT<MaxObjectNameLength> m_name;

private:

	class CStateDebugDrawData final
	{
	public:

		CStateDebugDrawData(SwitchStateId const audioSwitchState);

		CStateDebugDrawData(CStateDebugDrawData const&) = delete;
		CStateDebugDrawData(CStateDebugDrawData&&) = delete;
		CStateDebugDrawData& operator=(CStateDebugDrawData const&) = delete;
		CStateDebugDrawData& operator=(CStateDebugDrawData&&) = delete;

		void                 Update(SwitchStateId const audioSwitchState);

		SwitchStateId m_currentState;
		float         m_currentAlpha;

	private:

		static float const s_maxAlpha;
		static float const s_minAlpha;
		static int const   s_maxToMinUpdates;
	};

	typedef std::map<ControlId, CStateDebugDrawData> StateDrawInfoMap;

	mutable StateDrawInfoMap m_stateDrawInfoMap;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
