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
	SAudioTriggerImplState()
		: flags(eAudioTriggerStatus_None)
	{}

	AudioEnumFlagsType flags;
};

struct SUserDataBase
{
	SUserDataBase()
		: pOwnerOverride(nullptr)
		, pUserData(nullptr)
		, pUserDataOwner(nullptr)
	{}

	explicit SUserDataBase(
	  void* const _pOwnerOverride,
	  void* const _pUserData,
	  void* const _pUserDataOwner)
		: pOwnerOverride(_pOwnerOverride)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
	{}

	void* pOwnerOverride;
	void* pUserData;
	void* pUserDataOwner;
};

struct SAudioTriggerInstanceState : public SUserDataBase
{
	SAudioTriggerInstanceState()
		: flags(eAudioTriggerStatus_None)
		, audioTriggerId(INVALID_AUDIO_CONTROL_ID)
		, numPlayingEvents(0)
		, numLoadingEvents(0)
		, expirationTimeMS(0.0f)
		, remainingTimeMS(0.0f)
	{}

	AudioEnumFlagsType flags;
	AudioControlId     audioTriggerId;
	size_t             numPlayingEvents;
	size_t             numLoadingEvents;
	float              expirationTimeMS;
	float              remainingTimeMS;
};

struct SAudioStandaloneFileData : public SUserDataBase
{
	SAudioStandaloneFileData()
		: SUserDataBase(nullptr, nullptr, nullptr)
		, fileId(INVALID_AUDIO_STANDALONE_FILE_ID)
	{}

	explicit SAudioStandaloneFileData(
	  void* const _pOwner,
	  void* const _pUserData,
	  void* const _pUserDataOwner,
	  AudioStandaloneFileId const _fileId)
		: SUserDataBase(_pOwner, _pUserData, _pUserDataOwner)
		, fileId(_fileId)
	{}

	AudioStandaloneFileId const fileId;
};

// CATLAudioObject-related typedefs
typedef std::map<AudioStandaloneFileId, SAudioStandaloneFileData, std::less<AudioStandaloneFileId>,
                 STLSoundAllocator<std::pair<AudioStandaloneFileId, SAudioStandaloneFileData>>> ObjectStandaloneFileMap;

typedef std::set<CATLEvent*, std::less<CATLEvent*>, STLSoundAllocator<CATLEvent*>> ObjectEventSet;

typedef std::map<AudioTriggerImplId, SAudioTriggerImplState, std::less<AudioTriggerImplId>,
                 STLSoundAllocator<std::pair<AudioTriggerImplId, SAudioTriggerImplState>>> ObjectTriggerImplStates;

typedef std::map<AudioTriggerInstanceId, SAudioTriggerInstanceState, std::less<AudioTriggerInstanceId>,
                 STLSoundAllocator<std::pair<AudioTriggerInstanceId, SAudioTriggerInstanceState>>> ObjectTriggerStates;

typedef std::map<AudioControlId, AudioSwitchStateId, std::less<AudioControlId>,
                 STLSoundAllocator<std::pair<AudioControlId, AudioSwitchStateId>>> ObjectStateMap;

typedef std::map<AudioControlId, float, std::less<AudioControlId>,
                 STLSoundAllocator<std::pair<AudioControlId, float>>> ObjectRtpcMap;

typedef std::map<AudioEnvironmentId, float, std::less<AudioEnvironmentId>,
                 STLSoundAllocator<std::pair<AudioEnvironmentId, float>>> ObjectEnvironmentMap;

class CATLAudioObject final : public TATLObject
{
public:

	explicit CATLAudioObject(AudioObjectId const audioObjectId, CryAudio::Impl::IAudioObject* const pImplData = nullptr, EAudioDataScope const dataScope = eAudioDataScope_None)
		: TATLObject(audioObjectId, dataScope)
		, m_pImplData(pImplData)
		, m_flags(eAudioObjectFlags_None)
		, m_maxRadius(0.0f)
		, m_previousVelocity(0.0f)
		, m_propagationProcessor(audioObjectId, m_attributes.transformation) {}

	~CATLAudioObject() {}

	void Release();

	void ReportStartingTriggerInstance(AudioTriggerInstanceId const audioTriggerInstanceId, AudioControlId const audioTriggerId);
	void ReportStartedTriggerInstance(AudioTriggerInstanceId const audioTriggerInstanceId, void* const pOwnerOverride, void* const pUserData, void* const pUserDataOwner, AudioEnumFlagsType const flags);

	void ReportStartedEvent(CATLEvent* const _pEvent);
	void ReportFinishedEvent(CATLEvent* const _pEvent, bool const _bSuccess);

	void GetStartedStandaloneFileRequestData(CATLStandaloneFile const* const _pStandaloneFile, CAudioRequestInternal& _request);
	void ReportStartedStandaloneFile(
	  CATLStandaloneFile const* const _pStandaloneFile,
	  void* const _pOwner,
	  void* const _pUserData,
	  void* const _pUserDataOwner);

	void                                            ReportFinishedStandaloneFile(CATLStandaloneFile const* const _pStandaloneFile);

	void                                            ReportPrepUnprepTriggerImpl(AudioTriggerImplId const audioTriggerImplId, bool const bPrepared);

	void                                            SetSwitchState(AudioControlId const audioSwitchId, AudioSwitchStateId const audioSwitchStateId);
	void                                            SetRtpc(AudioControlId const audioRtpcId, float const value);
	void                                            SetEnvironmentAmount(AudioEnvironmentId const audioEnvironmentId, float const amount);

	ObjectTriggerImplStates const&                  GetTriggerImpls() const;
	ObjectRtpcMap const&                            GetRtpcs() const        { return m_rtpcs; }
	ObjectEnvironmentMap const&                     GetEnvironments() const;
	ObjectStateMap const&                           GetSwitchStates() const { return m_switchStates; }
	void                                            ClearEnvironments();

	ObjectEventSet const&                           GetActiveEvents() const                                       { return m_activeEvents; }
	ObjectStandaloneFileMap const&                  GetActiveStandaloneFiles() const                              { return m_activeStandaloneFiles; }
	ObjectTriggerStates&                            GetTriggerStates()                                            { return m_triggerStates; }

	void                                            SetImplDataPtr(CryAudio::Impl::IAudioObject* const pImplData) { m_pImplData = pImplData; }
	CryAudio::Impl::IAudioObject*                   GetImplDataPtr() const                                        { return m_pImplData; }

	float                                           GetMaxRadius() const                                          { return m_maxRadius; }

	void                                            Update(float const deltaTime, float const distance, Vec3 const& audioListenerPosition);
	void                                            Clear();
	void                                            ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo);
	CryAudio::Impl::SAudioObject3DAttributes const& Get3DAttributes() const { return m_attributes; }
	void                                            SetTransformation(CAudioObjectTransformation const& transformation);
	void                                            SetOcclusionType(EAudioOcclusionType const calcType, Vec3 const& audioListenerPosition);
	bool                                            CanRunObstructionOcclusion() const { return m_propagationProcessor.CanRunObstructionOcclusion(); }
	bool                                            HasNewOcclusionValues()            { return m_propagationProcessor.HasNewOcclusionValues(); }
	void                                            GetPropagationData(SATLSoundPropagationData& propagationData);
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
	ObjectRtpcMap                            m_rtpcs;
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

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void CheckBeforeRemoval(CATLDebugNameStore const* const pDebugNameStore) const;
	void DrawDebugInfo(IRenderAuxGeom& auxGeom, Vec3 const& listenerPosition, CATLDebugNameStore const* const pDebugNameStore) const;
	void ResetObstructionRays() { m_propagationProcessor.ResetRayData(); }

private:

	class CStateDebugDrawData
	{
	public:

		explicit CStateDebugDrawData(AudioSwitchStateId const audioSwitchState = INVALID_AUDIO_SWITCH_STATE_ID);
		~CStateDebugDrawData();

		void Update(AudioSwitchStateId const newState);

		AudioSwitchStateId m_currentState;
		float              m_currentAlpha;

	private:

		static float const s_maxAlpha;
		static float const s_minAlpha;
		static int const   s_maxToMinUpdates;
	};

	typedef std::map<AudioControlId, CStateDebugDrawData, std::less<AudioControlId>,
	                 STLSoundAllocator<std::pair<AudioControlId, CStateDebugDrawData>>> StateDrawInfoMap;

	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> GetTriggerNames(char const* const _szSeparator, CATLDebugNameStore const* const _pDebugNameStore) const;
	CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH>  GetEventIDs(char const* const _szSeparator) const;
	CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH>  GetStandaloneFileIDs(char const* const _szSeparator, CATLDebugNameStore const* const _pDebugNameStore) const;

	mutable StateDrawInfoMap m_stateDrawInfoMap;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	DELETE_DEFAULT_CONSTRUCTOR(CATLAudioObject);
	PREVENT_OBJECT_COPY(CATLAudioObject);
};
