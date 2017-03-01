// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../CryMath/Cry_Math.h"
#include "../CryCore/BaseTypes.h"
#include "../CryCore/smartptr.h"
#include "../CryCore/Platform/platform.h"

#define AUDIO_SYSTEM_DATA_ROOT "audio"

namespace CryAudio
{
typedef uint32 IdType;
typedef IdType ControlId;
typedef IdType SwitchStateId;
typedef IdType EnvironmentId;
typedef IdType PreloadRequestId;
typedef IdType FileEntryId;
typedef IdType TriggerImplId;
typedef IdType TriggerInstanceId;
typedef IdType EnumFlagsType;
typedef IdType AuxObjectId;

static constexpr ControlId InvalidControlId = 0;
static constexpr SwitchStateId InvalidSwitchStateId = 0;
static constexpr EnvironmentId InvalidEnvironmentId = 0;
static constexpr PreloadRequestId InvalidPreloadRequestId = 0;
static constexpr FileEntryId InvalidFileEntryId = 0;
static constexpr TriggerImplId InvalidTriggerImplId = 0;
static constexpr TriggerInstanceId InvalidTriggerInstanceId = 0;
static constexpr EnumFlagsType InvalidEnumFlagType = 0;
static constexpr AuxObjectId InvalidAuxObjectId = 0;
static constexpr AuxObjectId DefaultAuxObjectId = 1;

static constexpr uint8 MaxControlNameLength = 128;
static constexpr uint8 MaxFileNameLength = 128;
static constexpr uint16 MaxFilePathLength = 256;
static constexpr uint16 MaxObjectNameLength = 256;
static constexpr uint16 MaxMiscStringLength = 512;

// Forward declarations.
struct IObject;
class CATLEvent;
class CATLStandaloneFile;

enum ERequestFlags : EnumFlagsType
{
	eRequestFlags_None                 = 0,
	eRequestFlags_ExecuteBlocking      = BIT(0), // Blocks the calling thread until the requests has been processed.
	eRequestFlags_SyncCallback         = BIT(1), // Issues a callback on the same thread as the caller is on.
	eRequestFlags_SyncFinishedCallback = BIT(2), // Issues a callback upon a trigger instance finishing playback of all connected events on the same thread as the caller is on.
};

/**
 * An enum that lists possible statuses of an AudioRequest.
 * Used as a return type for many function used by the AudioSystem internally,
 * and also for most of the IAudioImpl calls
 */
enum ERequestStatus : EnumFlagsType
{
	eRequestStatus_None                    = 0,
	eRequestStatus_Success                 = 1,
	eRequestStatus_PartialSuccess          = 2,
	eRequestStatus_Failure                 = 3,
	eRequestStatus_Pending                 = 4,
	eRequestStatus_FailureInvalidObjectId  = 5,
	eRequestStatus_FailureInvalidControlId = 6,
	eRequestStatus_FailureInvalidRequest   = 7,
};

enum ERequestResult : EnumFlagsType
{
	eRequestResult_None,
	eRequestResult_Success,
	eRequestResult_Failure,
};

class CObjectTransformation
{
public:

	CObjectTransformation()
		: m_position(ZERO)
		, m_forward(Vec3Constants<float>::fVec3_OneY)
		, m_up(Vec3Constants<float>::fVec3_OneZ)
	{}

	CObjectTransformation(Vec3 const& position)
		: m_position(position)
		, m_forward(Vec3Constants<float>::fVec3_OneY)
		, m_up(Vec3Constants<float>::fVec3_OneZ)
	{}

	CObjectTransformation(Matrix34 const& transformation)
		: m_position(transformation.GetColumn3())
		, m_forward(transformation.GetColumn1())   //!< Assuming forward vector = (0,1,0), also assuming unscaled.
		, m_up(transformation.GetColumn2())        //!< Assuming up vector = (0,0,1).
	{
		m_forward.NormalizeFast();
		m_up.NormalizeFast();
	}

	ILINE Vec3 const& GetPosition() const { return m_position; }
	ILINE Vec3 const& GetForward() const  { return m_forward; }
	ILINE Vec3 const& GetUp() const       { return m_up; }

	bool              IsEquivalent(CObjectTransformation const& transformation, float const epsilon = VEC_EPSILON) const
	{
		return m_position.IsEquivalent(transformation.GetPosition(), epsilon) &&
		       m_forward.IsEquivalent(transformation.GetForward(), epsilon) &&
		       m_up.IsEquivalent(transformation.GetUp(), epsilon);
	}

	bool IsEquivalent(Matrix34 const& transformation, float const epsilon = VEC_EPSILON) const
	{
		return m_position.IsEquivalent(transformation.GetColumn3(), epsilon) &&
		       m_forward.IsEquivalent(transformation.GetColumn1(), epsilon) &&
		       m_up.IsEquivalent(transformation.GetColumn2(), epsilon);
	}

private:

	Vec3 m_position;
	Vec3 m_forward;
	Vec3 m_up;
};

static const CObjectTransformation s_nullAudioObjectTransformation;

struct SRequestUserData
{
	explicit SRequestUserData(
	  EnumFlagsType const _flags = eRequestFlags_None,
	  void* const _pOwner = nullptr,
	  void* const _pUserData = nullptr,
	  void* const _pUserDataOwner = nullptr)
		: flags(_flags)
		, pOwner(_pOwner)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
	{}

	static SRequestUserData const& GetEmptyObject() { static SRequestUserData const emptyInstance; return emptyInstance; }

	EnumFlagsType const            flags;
	void* const                    pOwner;
	void* const                    pUserData;
	void* const                    pUserDataOwner;
};

struct SRequestInfo
{
	explicit SRequestInfo(
	  ERequestResult const _requestResult,
	  void* const _pOwner,
	  void* const _pUserData,
	  void* const _pUserDataOwner,
	  EnumFlagsType const _audioSystemEvent,
	  ControlId const _audioControlId,
	  IObject* const _pAudioObject,
	  CATLStandaloneFile* _pStandaloneFile,
	  CATLEvent* _pAudioEvent)
		: requestResult(_requestResult)
		, pOwner(_pOwner)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
		, audioSystemEvent(_audioSystemEvent)
		, audioControlId(_audioControlId)
		, pAudioObject(_pAudioObject)
		, pStandaloneFile(_pStandaloneFile)
		, pAudioEvent(_pAudioEvent)
	{}

	SRequestInfo(SRequestInfo const&) = delete;
	SRequestInfo(SRequestInfo&&) = delete;
	SRequestInfo& operator=(SRequestInfo const&) = delete;
	SRequestInfo& operator=(SRequestInfo&&) = delete;

	ERequestResult const requestResult;
	void* const          pOwner;
	void* const          pUserData;
	void* const          pUserDataOwner;
	EnumFlagsType const  audioSystemEvent;
	ControlId const      audioControlId;
	IObject* const       pAudioObject;
	CATLStandaloneFile*  pStandaloneFile;
	CATLEvent*           pAudioEvent;
};

struct SFileData
{
	SFileData() = default;
	SFileData(SFileData const&) = delete;
	SFileData(SFileData&&) = delete;
	SFileData& operator=(SFileData const&) = delete;
	SFileData& operator=(SFileData&&) = delete;

	float duration = 0.0f;
};

struct STriggerData
{
	STriggerData() = default;
	STriggerData(STriggerData const&) = delete;
	STriggerData(STriggerData&&) = delete;
	STriggerData& operator=(STriggerData const&) = delete;
	STriggerData& operator=(STriggerData&&) = delete;

	float radius = 0.0f;
	float occlusionFadeOutDistance = 0.0f;
};

struct SPlayFileInfo
{
	explicit SPlayFileInfo(
	  char const* const _szFile,
	  bool const _bLocalized = true,
	  ControlId const _usedPlaybackTrigger = InvalidControlId)
		: szFile(_szFile)
		, bLocalized(_bLocalized)
		, usedTriggerForPlayback(_usedPlaybackTrigger)
	{}

	char const* const szFile;
	bool const        bLocalized;
	ControlId const   usedTriggerForPlayback;
};
} // namespace CryAudio
