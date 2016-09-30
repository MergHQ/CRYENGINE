// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLUtils.h"
#include "AudioInternalInterfaces.h"
#include <SharedAudioData.h>
#include <CrySystem/IStreamEngine.h>
#include <CrySystem/TimeValue.h>

class CATLAudioObject;

struct SATLXMLTags
{
	static char const* const szPlatform;

	static char const* const szRootNodeTag;
	static char const* const szEditorDataTag;
	static char const* const szTriggersNodeTag;
	static char const* const szRtpcsNodeTag;
	static char const* const szSwitchesNodeTag;
	static char const* const szPreloadsNodeTag;
	static char const* const szEnvironmentsNodeTag;

	static char const* const szATLTriggerTag;
	static char const* const szATLSwitchTag;
	static char const* const szATLRtpcTag;
	static char const* const szATLSwitchStateTag;
	static char const* const szATLEnvironmentTag;
	static char const* const szATLPlatformsTag;
	static char const* const szATLConfigGroupTag;

	static char const* const szATLTriggerRequestTag;
	static char const* const szATLSwitchRequestTag;
	static char const* const szATLValueTag;
	static char const* const szATLRtpcRequestTag;
	static char const* const szATLPreloadRequestTag;
	static char const* const szATLEnvironmentRequestTag;

	static char const* const szATLVersionAttribute;
	static char const* const szATLNameAttribute;
	static char const* const szATLInternalNameAttribute;
	static char const* const szATLTypeAttribute;
	static char const* const szATLConfigGroupAttribute;
	static char const* const szATLRadiusAttribute;
	static char const* const szATLOcclusionFadeOutDistanceAttribute;

	static char const* const szATLDataLoadType;
};

struct SATLInternalControlIDs
{
	static AudioControlId        obstructionOcclusionCalcSwitchId;
	static AudioControlId        objectDopplerTrackingSwitchId;
	static AudioControlId        objectVelocityTrackingSwitchId;
	static AudioControlId        loseFocusTriggerId;
	static AudioControlId        getFocusTriggerId;
	static AudioControlId        muteAllTriggerId;
	static AudioControlId        unmuteAllTriggerId;
	static AudioControlId        objectDopplerRtpcId;
	static AudioControlId        objectVelocityRtpcId;
	static AudioSwitchStateId    ignoreStateId;
	static AudioSwitchStateId    adaptiveStateId;
	static AudioSwitchStateId    lowStateId;
	static AudioSwitchStateId    mediumStateId;
	static AudioSwitchStateId    highStateId;
	static AudioSwitchStateId    onStateId;
	static AudioSwitchStateId    offStateId;
	static AudioPreloadRequestId globalPreloadRequestId;
};

void InitATLControlIDs(); // initializes the values in SATLInternalControlIDs

namespace CryAudio
{
namespace Impl
{
struct IAudioObject;
struct IAudioListener;
struct IAudioTrigger;
struct IAudioRtpc;
struct IAudioSwitchState;
struct IAudioEnvironment;
struct IAudioEvent;
struct IAudioFileEntry;
struct IAudioImpl;
}
}

enum EAudioObjectFlags : AudioEnumFlagsType
{
	eAudioObjectFlags_None                            = 0,
	eAudioObjectFlags_TrackDoppler                    = BIT(0),
	eAudioObjectFlags_TrackVelocity                   = BIT(1),
	eAudioObjectFlags_NeedsDopplerUpdate              = BIT(2),
	eAudioObjectFlags_NeedsVelocityUpdate             = BIT(3),
	eAudioObjectFlags_DoNotRelease                    = BIT(4),
	eAudioObjectFlags_Virtual                         = BIT(5),
	eAudioObjectFlags_WaitingForInitialTransformation = BIT(6),
};

enum EAudioSubsystem : AudioEnumFlagsType
{
	eAudioSubsystem_None          = 0,
	eAudioSubsystem_AudioImpl     = 1,
	eAudioSubsystem_AudioInternal = 2,
};

enum EAudioFileFlags : AudioEnumFlagsType
{
	eAudioFileFlags_Cached                    = BIT(0),
	eAudioFileFlags_NotCached                 = BIT(1),
	eAudioFileFlags_NotFound                  = BIT(2),
	eAudioFileFlags_MemAllocFail              = BIT(3),
	eAudioFileFlags_Removable                 = BIT(4),
	eAudioFileFlags_Loading                   = BIT(5),
	eAudioFileFlags_UseCounted                = BIT(6),
	eAudioFileFlags_NeedsResetToManualLoading = BIT(7),
	eAudioFileFlags_Localized                 = BIT(8),
};

template<typename IDType>
class CATLEntity
{
public:

	explicit CATLEntity(IDType const id, EAudioDataScope const dataScope)
		: m_id(id)
		, m_dataScope(dataScope)
	{}

	CATLEntity(CATLEntity const&) = delete;
	CATLEntity(CATLEntity&&) = delete;
	CATLEntity&             operator=(CATLEntity const&) = delete;
	CATLEntity&             operator=(CATLEntity&&) = delete;

	virtual IDType          GetId() const        { return m_id; }
	virtual EAudioDataScope GetDataScope() const { return m_dataScope; }

protected:

	~CATLEntity() = default;
	EAudioDataScope m_dataScope;

private:

	IDType const m_id;
};

typedef CATLEntity<AudioControlId> TATLControl;

struct SATLSoundPropagationData
{
	float obstruction = 0.0f;
	float occlusion = 0.0f;
};

class CATLListener final
{
public:

	CATLListener(CATLListener const&) = delete;
	CATLListener(CATLListener&&) = delete;
	CATLListener& operator=(CATLListener const&) = delete;
	CATLListener& operator=(CATLListener&&) = delete;

	explicit CATLListener(CryAudio::Impl::IAudioListener* const pImplData = nullptr)
		: m_pImplData(pImplData)
		, m_bNeedsFinalSetPosition(false)
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		, m_velocity(0.0f)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	{}

	void Update(CryAudio::Impl::IAudioImpl* const pImpl)
	{
		// Exponential decay towards zero.
		if (m_attributes.velocity.GetLengthSquared() > 0.0f)
		{
			float const deltaTime = (g_lastMainThreadFrameStartTime - m_previousTime).GetSeconds();
			float const decay = std::max(1.0f - deltaTime / 0.125f, 0.0f);
			m_attributes.velocity *= decay;
		}
		else if (m_bNeedsFinalSetPosition)
		{
			m_attributes.velocity = ZERO;
			pImpl->SetListener3DAttributes(m_pImplData, m_attributes);
			m_bNeedsFinalSetPosition = false;
		}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		m_velocity = m_attributes.velocity.GetLength();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	float GetVelocity() const { return m_velocity; }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	void SetTransformation(CAudioObjectTransformation const& transformation)
	{
		float const deltaTime = (g_lastMainThreadFrameStartTime - m_previousTime).GetSeconds();

		if (deltaTime > 0.0f)
		{
			m_attributes.transformation = transformation;
			m_attributes.velocity = (m_attributes.transformation.GetPosition() - m_previousAttributes.transformation.GetPosition()) / deltaTime;
			m_previousTime = g_lastMainThreadFrameStartTime;
			m_previousAttributes = m_attributes;
			m_bNeedsFinalSetPosition = m_attributes.velocity.GetLengthSquared() > 0.0f;
		}
		else if (deltaTime < 0.0f) //to handle time resets (e.g. loading a savegame might revert the gametime to a previous value)
		{
			m_attributes.transformation = transformation;
			m_previousTime = g_lastMainThreadFrameStartTime;
			m_previousAttributes = m_attributes;
			m_bNeedsFinalSetPosition = m_attributes.velocity.GetLengthSquared() > 0.0f;
		}
	}
	CryAudio::Impl::SAudioObject3DAttributes const& Get3DAttributes() const { return m_attributes; }

	CryAudio::Impl::IAudioListener* const m_pImplData;

private:

	bool m_bNeedsFinalSetPosition;
	CryAudio::Impl::SAudioObject3DAttributes m_attributes;
	CryAudio::Impl::SAudioObject3DAttributes m_previousAttributes;
	CTimeValue                               m_previousTime;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	float m_velocity;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CATLControlImpl
{
public:

	CATLControlImpl(CATLControlImpl const&) = delete;
	CATLControlImpl(CATLControlImpl&&) = delete;
	CATLControlImpl& operator=(CATLControlImpl const&) = delete;
	CATLControlImpl& operator=(CATLControlImpl&&) = delete;

	EAudioSubsystem  GetReceiver() const { return m_audioSubsystem; }

protected:

	CATLControlImpl() = default;

	explicit CATLControlImpl(EAudioSubsystem const audioSubsystem)
		: m_audioSubsystem(audioSubsystem)
	{}

	~CATLControlImpl() = default;

	EAudioSubsystem m_audioSubsystem = eAudioSubsystem_None;
};

class CATLTriggerImpl final : public CATLControlImpl
{
public:

	explicit CATLTriggerImpl(
	  AudioTriggerImplId const audioTriggerImplId,
	  AudioControlId const audioTriggerId,
	  EAudioSubsystem const audioSubsystem,
	  CryAudio::Impl::IAudioTrigger const* const pImplData = nullptr)
		: CATLControlImpl(audioSubsystem)
		, m_audioTriggerImplId(audioTriggerImplId)
		, m_audioTriggerId(audioTriggerId)
		, m_pImplData(pImplData)
	{}

	AudioTriggerImplId const                   m_audioTriggerImplId;
	AudioControlId const                       m_audioTriggerId;
	CryAudio::Impl::IAudioTrigger const* const m_pImplData;
};

class CATLTrigger final : public TATLControl
{
public:

	typedef std::vector<CATLTriggerImpl const*, STLSoundAllocator<CATLTriggerImpl const*>> ImplPtrVec;

	explicit CATLTrigger(
	  AudioControlId const audioTriggerId,
	  EAudioDataScope const dataScope,
	  ImplPtrVec const& implPtrs,
	  float const maxRadius,
	  float const occlusionFadeOutDistance)
		: TATLControl(audioTriggerId, dataScope)
		, m_implPtrs(implPtrs)
		, m_maxRadius(maxRadius)
		, m_occlusionFadeOutDistance(occlusionFadeOutDistance)
	{}

	ImplPtrVec const m_implPtrs;
	float const      m_maxRadius;
	float const      m_occlusionFadeOutDistance;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CATLRtpcImpl final : public CATLControlImpl
{
public:

	explicit CATLRtpcImpl(EAudioSubsystem const audioSubsystem, CryAudio::Impl::IAudioRtpc const* const pImplData = nullptr)
		: CATLControlImpl(audioSubsystem)
		, m_pImplData(pImplData)
	{}

	CryAudio::Impl::IAudioRtpc const* const m_pImplData;
};

class CATLRtpc final : public TATLControl
{
public:

	typedef std::vector<CATLRtpcImpl const*, STLSoundAllocator<CATLRtpcImpl const*>> ImplPtrVec;

	explicit CATLRtpc(AudioControlId const audioRtpcId, EAudioDataScope const dataScope, ImplPtrVec const& cImplPtrs)
		: TATLControl(audioRtpcId, dataScope)
		, m_implPtrs(cImplPtrs)
	{}

	ImplPtrVec const m_implPtrs;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CATLSwitchStateImpl final : public CATLControlImpl
{
public:

	explicit CATLSwitchStateImpl(EAudioSubsystem const audioSubsystem, CryAudio::Impl::IAudioSwitchState const* const pImplData = nullptr)
		: CATLControlImpl(audioSubsystem)
		, m_pImplData(pImplData)
	{}

	CryAudio::Impl::IAudioSwitchState const* const m_pImplData;
};

class CATLSwitchState final
{
public:

	typedef std::vector<CATLSwitchStateImpl const*, STLSoundAllocator<CATLSwitchStateImpl const*>> ImplPtrVec;

	explicit CATLSwitchState(
	  AudioControlId const audioSwitchId,
	  AudioSwitchStateId const audioSwitchStateId,
	  ImplPtrVec const& implPtrs)
		: m_audioSwitchStateId(audioSwitchStateId)
		, m_audioSwitchId(audioSwitchId)
		, m_implPtrs(implPtrs)
	{}

	CATLSwitchState(CATLSwitchState const&) = delete;
	CATLSwitchState(CATLSwitchState&&) = delete;
	CATLSwitchState&   operator=(CATLSwitchState const&) = delete;
	CATLSwitchState&   operator=(CATLSwitchState&&) = delete;

	AudioSwitchStateId GetId() const       { return m_audioSwitchStateId; }
	AudioSwitchStateId GetParentId() const { return m_audioSwitchId; }

	ImplPtrVec const m_implPtrs;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

private:

	AudioSwitchStateId const m_audioSwitchStateId;
	AudioControlId const     m_audioSwitchId;
};

class CATLSwitch final : public TATLControl
{
public:

	explicit CATLSwitch(AudioControlId const audioSwitchId, EAudioDataScope const dataScope)
		: TATLControl(audioSwitchId, dataScope)
	{}

	typedef std::map<AudioSwitchStateId, CATLSwitchState const*, std::less<AudioSwitchStateId>, STLSoundAllocator<std::pair<AudioSwitchStateId, CATLSwitchState const*>>> AudioStates;
	AudioStates audioSwitchStates;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CATLEnvironmentImpl final : public CATLControlImpl
{
public:

	explicit CATLEnvironmentImpl(EAudioSubsystem const audioSubsystem, CryAudio::Impl::IAudioEnvironment const* const pImplData = nullptr)
		: CATLControlImpl(audioSubsystem)
		, m_pImplData(pImplData)
	{}

	CryAudio::Impl::IAudioEnvironment const* const m_pImplData;
};

class CATLAudioEnvironment final : public CATLEntity<AudioEnvironmentId>
{
public:

	typedef std::vector<CATLEnvironmentImpl const*, STLSoundAllocator<CATLEnvironmentImpl const*>> ImplPtrVec;

	explicit CATLAudioEnvironment(AudioEnvironmentId const audioEnvironmentId, EAudioDataScope const dataScope, ImplPtrVec const& implPtrs)
		: CATLEntity<AudioEnvironmentId>(audioEnvironmentId, dataScope)
		, m_implPtrs(implPtrs)
	{}

	ImplPtrVec const m_implPtrs;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CATLStandaloneFile final : public CATLEntity<AudioStandaloneFileId>
{
public:

	explicit CATLStandaloneFile(AudioStandaloneFileId const instanceId, EAudioDataScope const dataScope)
		: CATLEntity<AudioStandaloneFileId>(instanceId, dataScope)
		, m_id(INVALID_AUDIO_STANDALONE_FILE_ID)
		, m_pAudioObject(nullptr)
		, m_pImplData(nullptr)
		, m_state(eAudioStandaloneFileState_None)
	{}

	bool         IsPlaying() const { return (m_state == eAudioStandaloneFileState_Playing) || (m_state == eAudioStandaloneFileState_Stopping); }

	virtual void Clear()
	{
		m_id = INVALID_AUDIO_STANDALONE_FILE_ID;
		m_pAudioObject = nullptr;
		m_state = eAudioStandaloneFileState_None;
	}

	AudioStandaloneFileId                 m_id;
	CATLAudioObject*                      m_pAudioObject;
	CryAudio::Impl::IAudioStandaloneFile* m_pImplData;
	EAudioStandaloneFileState             m_state;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CATLEvent final : public CATLEntity<AudioEventId>
{
public:

	explicit CATLEvent(AudioEventId const audioEventId, EAudioSubsystem const audioSubsystem)
		: CATLEntity<AudioEventId>(audioEventId, eAudioDataScope_None)
		, m_pAudioObject(nullptr)
		, m_pTrigger(nullptr)
		, m_audioTriggerImplId(INVALID_AUDIO_TRIGGER_IMPL_ID)
		, m_audioTriggerInstanceId(INVALID_AUDIO_TRIGGER_INSTANCE_ID)
		, m_audioEventState(eAudioEventState_None)
		, m_audioSubsystem(audioSubsystem)
		, m_pImplData(nullptr)
	{}

	void SetDataScope(EAudioDataScope const dataScope) { m_dataScope = dataScope; }
	bool IsPlaying() const                             { return m_audioEventState == eAudioEventState_Playing || m_audioEventState == eAudioEventState_PlayingDelayed; }

	void Clear()
	{
		m_dataScope = eAudioDataScope_None;
		m_pAudioObject = nullptr;
		m_pTrigger = nullptr;
		m_audioTriggerImplId = INVALID_AUDIO_TRIGGER_IMPL_ID;
		m_audioTriggerInstanceId = INVALID_AUDIO_TRIGGER_INSTANCE_ID;
		m_audioEventState = eAudioEventState_None;
	}

	CATLAudioObject*             m_pAudioObject;
	CATLTrigger const*           m_pTrigger;
	AudioTriggerImplId           m_audioTriggerImplId;
	AudioTriggerInstanceId       m_audioTriggerInstanceId;
	EAudioEventState             m_audioEventState;
	EAudioSubsystem const        m_audioSubsystem;
	CryAudio::Impl::IAudioEvent* m_pImplData;
};

class CATLAudioFileEntry final
{
public:

	explicit CATLAudioFileEntry(char const* const szPath = nullptr, CryAudio::Impl::IAudioFileEntry* const pImplData = nullptr)
		: m_path(szPath)
		, m_size(0)
		, m_useCount(0)
		, m_memoryBlockAlignment(AUDIO_MEMORY_ALIGNMENT)
		, m_flags(eAudioFileFlags_NotFound)
		, m_dataScope(eAudioDataScope_All)
		, m_streamTaskType(eStreamTaskTypeCount)
		, m_pMemoryBlock(nullptr)
		, m_pReadStream(nullptr)
		, m_pImplData(pImplData)
	{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		m_timeCached.SetValue(0);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	CATLAudioFileEntry(CATLAudioFileEntry const&) = delete;
	CATLAudioFileEntry(CATLAudioFileEntry&&) = delete;
	CATLAudioFileEntry& operator=(CATLAudioFileEntry const&) = delete;
	CATLAudioFileEntry& operator=(CATLAudioFileEntry&&) = delete;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> m_path;
	size_t                           m_size;
	size_t                           m_useCount;
	size_t                           m_memoryBlockAlignment;
	AudioEnumFlagsType               m_flags;
	EAudioDataScope                  m_dataScope;
	EStreamTaskType                  m_streamTaskType;
	_smart_ptr<ICustomMemoryBlock>   m_pMemoryBlock;
	IReadStreamPtr                   m_pReadStream;
	CryAudio::Impl::IAudioFileEntry* m_pImplData;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CTimeValue m_timeCached;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CATLPreloadRequest final : public CATLEntity<AudioPreloadRequestId>
{
public:

	typedef std::vector<AudioFileEntryId, STLSoundAllocator<AudioFileEntryId>> FileEntryIds;

	explicit CATLPreloadRequest(
	  AudioPreloadRequestId const audioPreloadRequestId,
	  EAudioDataScope const dataScope,
	  bool const bAutoLoad,
	  FileEntryIds const& fileEntryIds)
		: CATLEntity<AudioPreloadRequestId>(audioPreloadRequestId, dataScope)
		, m_bAutoLoad(bAutoLoad)
		, m_fileEntryIds(fileEntryIds)
	{}

	bool const   m_bAutoLoad;
	FileEntryIds m_fileEntryIds;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> m_name;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

//-------------------- ATLObject container typedefs --------------------------
typedef std::map<AudioControlId, CATLTrigger const*, std::less<AudioControlId>, STLSoundAllocator<std::pair<AudioControlId, CATLTrigger const*>>>                               AudioTriggerLookup;
typedef std::map<AudioControlId, CATLRtpc const*, std::less<AudioControlId>, STLSoundAllocator<std::pair<AudioControlId, CATLRtpc const*>>>                                     AudioRtpcLookup;
typedef std::map<AudioControlId, CATLSwitch const*, std::less<AudioControlId>, STLSoundAllocator<std::pair<AudioControlId, CATLSwitch const*>>>                                 AudioSwitchLookup;
typedef std::map<AudioPreloadRequestId, CATLPreloadRequest*, std::less<AudioPreloadRequestId>, STLSoundAllocator<std::pair<AudioPreloadRequestId, CATLPreloadRequest*>>>        AudioPreloadRequestLookup;
typedef std::map<AudioEnvironmentId, CATLAudioEnvironment const*, std::less<AudioEnvironmentId>, STLSoundAllocator<std::pair<AudioEnvironmentId, CATLAudioEnvironment const*>>> AudioEnvironmentLookup;
typedef std::map<AudioStandaloneFileId, CATLStandaloneFile*, std::less<AudioStandaloneFileId>, STLSoundAllocator<std::pair<AudioStandaloneFileId, CATLStandaloneFile*>>>        AudioStandaloneFileLookup;

//------------------------ ATLInternal Entity Data ----------------------------
struct SATLSwitchStateImplData_internal final : public CryAudio::Impl::IAudioSwitchState
{
	explicit SATLSwitchStateImplData_internal(AudioControlId const _internalAudioSwitchId, AudioSwitchStateId const _internalAudioSwitchStateId)
		: internalAudioSwitchId(_internalAudioSwitchId)
		, internalAudioSwitchStateId(_internalAudioSwitchStateId)
	{}

	virtual ~SATLSwitchStateImplData_internal() override = default;

	SATLSwitchStateImplData_internal(SATLSwitchStateImplData_internal const&) = delete;
	SATLSwitchStateImplData_internal(SATLSwitchStateImplData_internal&&) = delete;
	SATLSwitchStateImplData_internal& operator=(SATLSwitchStateImplData_internal const&) = delete;
	SATLSwitchStateImplData_internal& operator=(SATLSwitchStateImplData_internal&&) = delete;

	AudioControlId const     internalAudioSwitchId;
	AudioSwitchStateId const internalAudioSwitchStateId;
};
