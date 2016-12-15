// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Cry_Math.h>
#include <CryCore/BaseTypes.h>
#include <CryCore/smartptr.h>

#define AUDIO_SYSTEM_DATA_ROOT "audio"

typedef uint32 AudioIdType;

class CATLListener;
class CATLAudioObject;

typedef AudioIdType AudioControlId;
#define INVALID_AUDIO_CONTROL_ID              ((AudioControlId)(0))
typedef AudioIdType AudioSwitchStateId;
#define INVALID_AUDIO_SWITCH_STATE_ID         ((AudioSwitchStateId)(0))
typedef AudioIdType AudioEnvironmentId;
#define INVALID_AUDIO_ENVIRONMENT_ID          ((AudioEnvironmentId)(0))
typedef AudioIdType AudioPreloadRequestId;
#define INVALID_AUDIO_PRELOAD_REQUEST_ID      ((AudioPreloadRequestId)(0))
typedef AudioIdType AudioStandaloneFileId;
#define INVALID_AUDIO_STANDALONE_FILE_ID      ((AudioStandaloneFileId)(0))
typedef AudioIdType AudioEventId;
#define INVALID_AUDIO_EVENT_ID                ((AudioEventId)(0))
typedef AudioIdType AudioFileEntryId;
#define INVALID_AUDIO_FILE_ENTRY_ID           ((AudioFileEntryId)(0))
typedef AudioIdType AudioTriggerImplId;
#define INVALID_AUDIO_TRIGGER_IMPL_ID         ((AudioTriggerImplId)(0))
typedef AudioIdType AudioTriggerInstanceId;
#define INVALID_AUDIO_TRIGGER_INSTANCE_ID     ((AudioTriggerInstanceId)(0))
typedef AudioIdType AudioEnumFlagsType;
#define INVALID_AUDIO_ENUM_FLAG_TYPE          ((AudioEnumFlagsType)(0))
#define ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS ((AudioEnumFlagsType)(0xFFFFFFFF))
typedef AudioIdType AudioProxyId;
#define INVALID_AUDIO_PROXY_ID                ((AudioProxyId)(0))
#define DEFAULT_AUDIO_PROXY_ID                ((AudioProxyId)(1))

class CAudioObjectTransformation
{
public:

	CAudioObjectTransformation()
		: m_position(ZERO)
		, m_forward(Vec3Constants<float>::fVec3_OneY)
		, m_up(Vec3Constants<float>::fVec3_OneZ)
	{}

	CAudioObjectTransformation(Vec3 const& position)
		: m_position(position)
		, m_forward(Vec3Constants<float>::fVec3_OneY)
		, m_up(Vec3Constants<float>::fVec3_OneZ)
	{}

	CAudioObjectTransformation(Matrix34 const& transformation)
		: m_position(transformation.GetColumn3())
		, m_forward(transformation.GetColumn1()) //!< Assuming forward vector = (0,1,0), also assuming unscaled.
		, m_up(transformation.GetColumn2())      //!< Assuming up vector = (0,0,1).
	{
		m_forward.NormalizeFast();
		m_up.NormalizeFast();
	}

	ILINE Vec3 const& GetPosition() const { return m_position; }
	ILINE Vec3 const& GetForward() const  { return m_forward; }
	ILINE Vec3 const& GetUp() const       { return m_up; }

	bool              IsEquivalent(Matrix34 const& transformation, float const fEpsilon = VEC_EPSILON) const
	{
		return m_position.IsEquivalent(transformation.GetColumn3(), fEpsilon) &&
		       m_forward.IsEquivalent(transformation.GetColumn1(), fEpsilon) &&
		       m_up.IsEquivalent(transformation.GetColumn2(), fEpsilon);
	}

private:

	Vec3 m_position;
	Vec3 m_forward;
	Vec3 m_up;
};

#define AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED 100 // IDs below that value are used for the CATLTriggerImpl_Internal

#define MAX_AUDIO_CONTROL_NAME_LENGTH      128
#define MAX_AUDIO_FILE_NAME_LENGTH         128
#define MAX_AUDIO_FILE_PATH_LENGTH         256
#define MAX_AUDIO_OBJECT_NAME_LENGTH       256
#define MAX_AUDIO_MISC_STRING_LENGTH       512

enum EAudioRequestFlags : AudioEnumFlagsType
{
	eAudioRequestFlags_None                 = 0,
	eAudioRequestFlags_PriorityNormal       = BIT(0),
	eAudioRequestFlags_PriorityHigh         = BIT(1),
	eAudioRequestFlags_ExecuteBlocking      = BIT(2),
	eAudioRequestFlags_SyncCallback         = BIT(3),
	eAudioRequestFlags_SyncFinishedCallback = BIT(4),
	eAudioRequestFlags_StayInMemory         = BIT(5),
	eAudioRequestFlags_ThreadSafePush       = BIT(6),
};

enum EAudioRequestType : AudioEnumFlagsType
{
	eAudioRequestType_None,
	eAudioRequestType_AudioManagerRequest,
	eAudioRequestType_AudioCallbackManagerRequest,
	eAudioRequestType_AudioObjectRequest,
	eAudioRequestType_AudioListenerRequest,
	eAudioRequestType_AudioAllRequests = 0xFFFFFFFF,
};

enum EAudioRequestResult : AudioEnumFlagsType
{
	eAudioRequestResult_None,
	eAudioRequestResult_Success,
	eAudioRequestResult_Failure,
};

struct SAudioRequestDataBase
{
	explicit SAudioRequestDataBase(EAudioRequestType const _type)
		: type(_type)
	{}

	virtual ~SAudioRequestDataBase() = default;

	SAudioRequestDataBase(SAudioRequestDataBase const&) = delete;
	SAudioRequestDataBase(SAudioRequestDataBase&&) = delete;
	SAudioRequestDataBase& operator=(SAudioRequestDataBase const&) = delete;
	SAudioRequestDataBase& operator=(SAudioRequestDataBase&&) = delete;

	EAudioRequestType const type;
};

struct SAudioCallBackInfo
{
	SAudioCallBackInfo(SAudioCallBackInfo const& other)
		: pObjectToNotify(other.pObjectToNotify)
		, pUserData(other.pUserData)
		, pUserDataOwner(other.pUserDataOwner)
		, requestFlags(other.requestFlags)
	{}

	explicit SAudioCallBackInfo(
	  void* const _pObjectToNotify = nullptr,
	  void* const _pUserData = nullptr,
	  void* const _pUserDataOwner = nullptr,
	  AudioEnumFlagsType const _requestFlags = eAudioRequestFlags_PriorityNormal)
		: pObjectToNotify(_pObjectToNotify)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
		, requestFlags(_requestFlags)
	{}

	static SAudioCallBackInfo const& GetEmptyObject() { static SAudioCallBackInfo const emptyInstance; return emptyInstance; }

	void* const                      pObjectToNotify;
	void* const                      pUserData;
	void* const                      pUserDataOwner;
	AudioEnumFlagsType const         requestFlags;
};

struct SAudioPlayFileInfo
{
	explicit SAudioPlayFileInfo(
	  char const* const _szFile
	  , bool const _bLocalized = true
	  , AudioControlId const _usedPlaybackTrigger = INVALID_AUDIO_CONTROL_ID)
		: szFile(_szFile)
		, bLocalized(_bLocalized)
		, usedTriggerForPlayback(_usedPlaybackTrigger)
	{}

	char const* const    szFile;
	bool const           bLocalized;
	AudioControlId const usedTriggerForPlayback;
};

struct SAudioRequest
{
	SAudioRequest() = default;
	SAudioRequest(SAudioRequest const&) = delete;
	SAudioRequest(SAudioRequest&&) = delete;
	SAudioRequest& operator=(SAudioRequest const&) = delete;
	SAudioRequest& operator=(SAudioRequest&&) = delete;

	AudioEnumFlagsType     flags = eAudioRequestFlags_None;
	CATLAudioObject*       pAudioObject = nullptr;
	void*                  pOwner = nullptr;
	void*                  pUserData = nullptr;
	void*                  pUserDataOwner = nullptr;
	SAudioRequestDataBase* pData = nullptr;
};

struct SAudioRequestInfo
{
	explicit SAudioRequestInfo(
	  EAudioRequestResult const _requestResult,
	  void* const _pOwner,
	  void* const _pUserData,
	  void* const _pUserDataOwner,
	  char const* const _szValue,
	  EAudioRequestType const _audioRequestType,
	  AudioEnumFlagsType const _specificAudioRequest,
	  AudioControlId const _audioControlId,
	  CATLAudioObject* const _pAudioObject,
	  AudioStandaloneFileId const _audioStandaloneFileId,
	  AudioEventId const _audioEventId)
		: requestResult(_requestResult)
		, pOwner(_pOwner)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
		, szValue(_szValue)
		, audioRequestType(_audioRequestType)
		, specificAudioRequest(_specificAudioRequest)
		, audioControlId(_audioControlId)
		, pAudioObject(_pAudioObject)
		, audioStandaloneFileId(_audioStandaloneFileId)
		, audioEventId(_audioEventId)
	{}

	SAudioRequestInfo(SAudioRequestInfo const&) = delete;
	SAudioRequestInfo(SAudioRequestInfo&&) = delete;
	SAudioRequestInfo& operator=(SAudioRequestInfo const&) = delete;
	SAudioRequestInfo& operator=(SAudioRequestInfo&&) = delete;

	EAudioRequestResult const   requestResult;
	void* const                 pOwner;
	void* const                 pUserData;
	void* const                 pUserDataOwner;
	char const* const           szValue;
	EAudioRequestType const     audioRequestType;
	AudioEnumFlagsType const    specificAudioRequest;
	AudioControlId const        audioControlId;
	CATLAudioObject* const      pAudioObject;
	AudioStandaloneFileId const audioStandaloneFileId;
	AudioEventId const          audioEventId;
};

struct SAudioFileData
{
	SAudioFileData() = default;
	SAudioFileData(SAudioFileData const&) = delete;
	SAudioFileData(SAudioFileData&&) = delete;
	SAudioFileData& operator=(SAudioFileData const&) = delete;
	SAudioFileData& operator=(SAudioFileData&&) = delete;

	float duration = 0.0f;
};

struct SAudioTriggerData
{
	SAudioTriggerData() = default;
	SAudioTriggerData(SAudioTriggerData const&) = delete;
	SAudioTriggerData(SAudioTriggerData&&) = delete;
	SAudioTriggerData& operator=(SAudioTriggerData const&) = delete;
	SAudioTriggerData& operator=(SAudioTriggerData&&) = delete;

	float radius = 0.0f;
	float occlusionFadeOutDistance = 0.0f;
};
