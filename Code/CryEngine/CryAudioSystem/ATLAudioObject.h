// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include "PropagationProcessor.h"
#include <CrySystem/TimeValue.h>

struct IRenderAuxGeom;

enum EAudioTriggerStatus : AudioEnumFlagsType
{
	eAudioTriggerStatus_None                  = 0,
	eAudioTriggerStatus_Playing               = BIT(0),
	eAudioTriggerStatus_Prepared              = BIT(1),
	eAudioTriggerStatus_Loading               = BIT(2),
	eAudioTriggerStatus_Unloading             = BIT(3),
	eAudioTriggerStatus_Starting              = BIT(4),
	eAudioTriggerStatus_WaitingForRemoval     = BIT(5),
	eAudioTriggerStatus_CallbackOnAudioThread = BIT(6),
};

struct SAudioTriggerImplState
{
	AudioEnumFlagsType flags = eAudioTriggerStatus_None;
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

struct SAudioTriggerInstanceState : public SUserDataBase
{
	AudioEnumFlagsType flags = eAudioTriggerStatus_None;
	AudioControlId     audioTriggerId = INVALID_AUDIO_CONTROL_ID;
	size_t             numPlayingEvents = 0;
	size_t             numLoadingEvents = 0;
	float              expirationTimeMS = 0.0f;
	float              remainingTimeMS = 0.0f;
};

struct SAudioStandaloneFileData : public SUserDataBase
{
	SAudioStandaloneFileData() = default;

	explicit SAudioStandaloneFileData(
	  void* const pOwner,
	  void* const pUserData,
	  void* const pUserDataOwner,
	  AudioStandaloneFileId const fileId)
		: SUserDataBase(pOwner, pUserData, pUserDataOwner)
		, m_fileId(fileId)
	{}

	AudioStandaloneFileId const m_fileId = INVALID_AUDIO_STANDALONE_FILE_ID;
};

// CATLAudioObject-related typedefs
typedef std::map<AudioStandaloneFileId, SAudioStandaloneFileData, std::less<AudioStandaloneFileId>,
                 STLSoundAllocator<std::pair<AudioStandaloneFileId const, SAudioStandaloneFileData>>> ObjectStandaloneFileMap;

typedef std::set<CATLEvent*, std::less<CATLEvent*>, STLSoundAllocator<CATLEvent*>> ObjectEventSet;

typedef std::map<AudioTriggerImplId, SAudioTriggerImplState, std::less<AudioTriggerImplId>,
                 STLSoundAllocator<std::pair<AudioTriggerImplId const, SAudioTriggerImplState>>> ObjectTriggerImplStates;

typedef std::map<AudioTriggerInstanceId, SAudioTriggerInstanceState, std::less<AudioTriggerInstanceId>,
                 STLSoundAllocator<std::pair<AudioTriggerInstanceId const, SAudioTriggerInstanceState>>> ObjectTriggerStates;

typedef std::map<AudioControlId, AudioSwitchStateId, std::less<AudioControlId>,
                 STLSoundAllocator<std::pair<AudioControlId const, AudioSwitchStateId>>> ObjectStateMap;

typedef std::map<AudioControlId, float, std::less<AudioControlId>,
                 STLSoundAllocator<std::pair<AudioControlId const, float>>> ObjectRtpcMap;

typedef std::map<AudioEnvironmentId, float, std::less<AudioEnvironmentId>,
                 STLSoundAllocator<std::pair<AudioEnvironmentId const, float>>> ObjectEnvironmentMap;

class CATLAudioObject final
{
public:

	explicit CATLAudioObject(CryAudio::Impl::IAudioObject* const pImplData = nullptr);

	CATLAudioObject(CATLAudioObject const&) = delete;
	CATLAudioObject(CATLAudioObject&&) = delete;
	CATLAudioObject& operator=(CATLAudioObject const&) = delete;
	CATLAudioObject& operator=(CATLAudioObject&&) = delete;

	void             Release();

	void             ReportStartingTriggerInstance(AudioTriggerInstanceId const audioTriggerInstanceId, AudioControlId const audioTriggerId);
	void             ReportStartedTriggerInstance(AudioTriggerInstanceId const audioTriggerInstanceId, void* const pOwnerOverride, void* const pUserData, void* const pUserDataOwner, AudioEnumFlagsType const flags);

	void             ReportStartedEvent(CATLEvent* const _pEvent);
	void             ReportFinishedEvent(CATLEvent* const _pEvent, bool const _bSuccess);

	void             GetStartedStandaloneFileRequestData(CATLStandaloneFile const* const pStandaloneFile, CAudioRequestInternal& request);
	void             ReportStartedStandaloneFile(
	  CATLStandaloneFile const* const pStandaloneFile,
	  void* const pOwner,
	  void* const pUserData,
	  void* const pUserDataOwner);

	void                                            ReportFinishedStandaloneFile(CATLStandaloneFile const* const pStandaloneFile);

	void                                            ReportPrepUnprepTriggerImpl(AudioTriggerImplId const audioTriggerImplId, bool const bPrepared);

	void                                            SetSwitchState(AudioControlId const audioSwitchId, AudioSwitchStateId const audioSwitchStateId);
	void                                            SetParameter(AudioControlId const audioParameterId, float const value);
	void                                            SetEnvironmentAmount(AudioEnvironmentId const audioEnvironmentId, float const amount);

	ObjectTriggerImplStates const&                  GetTriggerImpls() const;
	ObjectRtpcMap const&                            GetParameter() const    { return m_parameters; }
	ObjectEnvironmentMap const&                     GetEnvironments() const;
	ObjectStateMap const&                           GetSwitchStates() const { return m_switchStates; }
	void                                            ClearEnvironments();

	ObjectEventSet const&                           GetActiveEvents() const                                       { return m_activeEvents; }
	ObjectStandaloneFileMap const&                  GetActiveStandaloneFiles() const                              { return m_activeStandaloneFiles; }
	ObjectTriggerStates&                            GetTriggerStates()                                            { return m_triggerStates; }

	void                                            SetImplDataPtr(CryAudio::Impl::IAudioObject* const pImplData) { m_pImplData = pImplData; }
	CryAudio::Impl::IAudioObject*                   GetImplDataPtr() const                                        { return m_pImplData; }

	float                                           GetMaxRadius() const                                          { return m_maxRadius; }
	float                                           GetOcclusionFadeOutDistance() const                           { return m_occlusionFadeOutDistance; }

	void                                            Update(float const deltaTime, float const distance, Vec3 const& audioListenerPosition);
	void                                            Clear();
	void                                            ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo);
	CryAudio::Impl::SAudioObject3DAttributes const& Get3DAttributes() const { return m_attributes; }
	void                                            SetTransformation(CAudioObjectTransformation const& transformation);
	void                                            SetOcclusionType(EAudioOcclusionType const calcType, Vec3 const& audioListenerPosition);
	bool                                            CanRunObstructionOcclusion() const { return m_propagationProcessor.CanRunObstructionOcclusion(); }
	bool                                            HasNewOcclusionValues()            { return m_propagationProcessor.HasNewOcclusionValues(); }
	void                                            GetPropagationData(SATLSoundPropagationData& propagationData) const;
	void                                            ReleasePendingRays();
	void                                            SetDopplerTracking(bool const bEnable);
	void                                            SetVelocityTracking(bool const bEnable);
	void                                            UpdateControls(float const deltaTime, CryAudio::Impl::SAudioObject3DAttributes const& listenerAttributes);
	bool                                            CanBeReleased() const;
	AudioEnumFlagsType                              GetFlags() const { return m_flags; }
	void                                            SetFlag(EAudioObjectFlags const flag);
	void                                            RemoveFlag(EAudioObjectFlags const flag);

private:

	void ReportFinishedTriggerInstance(ObjectTriggerStates::iterator& iter);

	ObjectStandaloneFileMap                  m_activeStandaloneFiles;
	ObjectEventSet                           m_activeEvents;
	ObjectTriggerStates                      m_triggerStates;
	ObjectTriggerImplStates                  m_triggerImplStates;
	ObjectRtpcMap                            m_parameters;
	ObjectEnvironmentMap                     m_environments;
	ObjectStateMap                           m_switchStates;
	CryAudio::Impl::IAudioObject*            m_pImplData;
	float                                    m_maxRadius;
	AudioEnumFlagsType                       m_flags;
	float                                    m_previousVelocity;
	Vec3                                     m_velocity;
	CryAudio::Impl::SAudioObject3DAttributes m_attributes;
	CryAudio::Impl::SAudioObject3DAttributes m_previousAttributes;
	CTimeValue                               m_previousTime;
	CPropagationProcessor                    m_propagationProcessor;
	float                                    m_occlusionFadeOutDistance;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(
	  IRenderAuxGeom& auxGeom,
	  Vec3 const& listenerPosition,
	  AudioTriggerLookup const& triggers,
	  AudioRtpcLookup const& parameters,
	  AudioSwitchLookup const& switches,
	  AudioPreloadRequestLookup const& preloadRequests,
	  AudioEnvironmentLookup const& environments,
	  AudioStandaloneFileLookup const& audioStandaloneFiles) const;
	void ResetObstructionRays() { m_propagationProcessor.ResetRayData(); }

	CryFixedStringT<MAX_AUDIO_OBJECT_NAME_LENGTH> m_name;

private:

	class CStateDebugDrawData final
	{
	public:

		CStateDebugDrawData(AudioSwitchStateId const audioSwitchState);

		CStateDebugDrawData(CStateDebugDrawData const&) = delete;
		CStateDebugDrawData(CStateDebugDrawData&&) = delete;
		CStateDebugDrawData& operator=(CStateDebugDrawData const&) = delete;
		CStateDebugDrawData& operator=(CStateDebugDrawData&&) = delete;

		void                 Update(AudioSwitchStateId const audioSwitchState);

		AudioSwitchStateId m_currentState;
		float              m_currentAlpha;

	private:

		static float const s_maxAlpha;
		static float const s_minAlpha;
		static int const   s_maxToMinUpdates;
	};

	typedef std::map<AudioControlId, CStateDebugDrawData, std::less<AudioControlId>,
	                 STLSoundAllocator<std::pair<AudioControlId const, CStateDebugDrawData>>> StateDrawInfoMap;

	mutable StateDrawInfoMap m_stateDrawInfoMap;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
