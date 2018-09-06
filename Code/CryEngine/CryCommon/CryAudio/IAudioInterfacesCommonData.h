// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../CryMath/Cry_Math.h"
#include "../CryCore/BaseTypes.h"
#include "../CryCore/smartptr.h"
#include "../CryCore/Platform/platform.h"
#include "../CryCore/CryEnumMacro.h"
#include "../CryCore/CryCrc32.h"

#define AUDIO_SYSTEM_DATA_ROOT "audio"

namespace CryAudio
{
using IdType = uint32;
using ControlId = IdType;
using SwitchStateId = IdType;
using EnvironmentId = IdType;
using PreloadRequestId = IdType;
using FileEntryId = IdType;
using TriggerImplId = IdType;
using TriggerInstanceId = IdType;
using EnumFlagsType = IdType;
using AuxObjectId = IdType;
using LibraryId = IdType;

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
static constexpr uint8 MaxInfoStringLength = 128;
static constexpr uint8 MaxControlNameLength = 128;
static constexpr uint8 MaxFileNameLength = 128;
static constexpr uint16 MaxFilePathLength = 256;
static constexpr uint16 MaxObjectNameLength = 256;
static constexpr uint16 MaxMiscStringLength = 512;
static constexpr uint32 InvalidCRC32 = 0xFFFFffff;
static constexpr float FloatEpsilon = 1.0e-3f;

static constexpr char const* s_szLoseFocusTriggerName = "lose_focus";
static constexpr char const* s_szGetFocusTriggerName = "get_focus";
static constexpr char const* s_szMuteAllTriggerName = "mute_all";
static constexpr char const* s_szUnmuteAllTriggerName = "unmute_all";
static constexpr char const* s_szPauseAllTriggerName = "pause_all";
static constexpr char const* s_szResumeAllTriggerName = "resume_all";
static constexpr char const* s_szGlobalPreloadRequestName = "global_audio_system_preload";

static constexpr char const* s_szDefaultLibraryName = "default_controls";
static constexpr char const* s_szRootNodeTag = "AudioSystemData";
static constexpr char const* s_szEditorDataTag = "EditorData";
static constexpr char const* s_szTriggersNodeTag = "Triggers";
static constexpr char const* s_szParametersNodeTag = "Parameters";
static constexpr char const* s_szSwitchesNodeTag = "Switches";
static constexpr char const* s_szPreloadsNodeTag = "Preloads";
static constexpr char const* s_szEnvironmentsNodeTag = "Environments";
static constexpr char const* s_szSettingsNodeTag = "Settings";

static constexpr char const* s_szTriggerTag = "Trigger";
static constexpr char const* s_szParameterTag = "Parameter";
static constexpr char const* s_szSwitchTag = "Switch";
static constexpr char const* s_szStateTag = "State";
static constexpr char const* s_szEnvironmentTag = "Environment";
static constexpr char const* s_szPreloadRequestTag = "PreloadRequest";
static constexpr char const* s_szSettingTag = "Setting";
static constexpr char const* s_szPlatformTag = "Platform";
static constexpr char const* s_szEventTag = "Event";

static constexpr char const* s_szNameAttribute = "name";
static constexpr char const* s_szVersionAttribute = "version";
static constexpr char const* s_szTypeAttribute = "type";

static constexpr char const* s_szDataLoadType = "autoload";

static constexpr char const* s_szConfigFolderName = "ace";
static constexpr char const* s_szAssetsFolderName = "assets";
static constexpr char const* s_szLevelsFolderName = "levels";

/**
 * A utility function to convert a string value to an Id.
 * @param szSource - string to convert
 * @return a 32bit CRC computed on the lower case version of the passed string
 */
static constexpr IdType StringToId(char const* const szSource)
{
	return static_cast<IdType>(CCrc32::ComputeLowercase_CompileTime(szSource));
}

static constexpr PreloadRequestId GlobalPreloadRequestId = StringToId(s_szGlobalPreloadRequestName);
static constexpr LibraryId DefaultLibraryId = StringToId(s_szDefaultLibraryName);

/**
 * @enum CryAudio::ERequestFlags
 * @brief A strongly typed enum class representing flags that can be passed into methods via the SRequestUserData parameter that control how an internally generated request behaves or what to do along with it.
 * @var CryAudio::ERequestFlags::None
 * @var CryAudio::ERequestFlags::ExecuteBlocking
 * @var CryAudio::ERequestFlags::CallbackOnExternalOrCallingThread
 * @var CryAudio::ERequestFlags::CallbackOnAudioThread
 * @var CryAudio::ERequestFlags::DoneCallbackOnExternalThread
 * @var CryAudio::ERequestFlags::DoneCallbackOnAudioThread
 */
enum class ERequestFlags : EnumFlagsType
{
	None,                                       /**< Used to initialize variables of this type. */
	ExecuteBlocking                   = BIT(0), /**< Blocks the calling thread until the request has been processed. */
	CallbackOnExternalOrCallingThread = BIT(1), /**< Invokes a callback on the calling thread for blocking requests or invokes a callback on the external thread for non-blocking requests. */
	CallbackOnAudioThread             = BIT(2), /**< Invokes a callback on the audio thread informing of the outcome of the request. */
	DoneCallbackOnExternalThread      = BIT(3), /**< Invokes a callback on the external thread once a trigger instance finished playback of all its events. */
	DoneCallbackOnAudioThread         = BIT(4), /**< Invokes a callback on the audio thread once a trigger instance finished playback of all its events. */
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ERequestFlags);

/**
 * @enum CryAudio::ERequestStatus
 * @brief A strongly typed enum class representing a list of possible statuses of an internally generated audio request. Used as a return type for many methods used by the AudioSystem internally and also for most of the CryAudio::Impl::IImpl calls.
 * @var CryAudio::ERequestStatus::None
 * @var CryAudio::ERequestStatus::ExecuteBlocking
 * @var CryAudio::ERequestStatus::CallbackOnExternalOrCallingThread
 * @var CryAudio::ERequestStatus::CallbackOnAudioThread
 * @var CryAudio::ERequestStatus::DoneCallbackOnExternalThread
 * @var CryAudio::ERequestStatus::DoneCallbackOnAudioThread
 */
enum class ERequestStatus : EnumFlagsType
{
	None,                    /**< Used to initialize variables of this type and to determine whether the variable was properly handled. */
	Success,                 /**< Returned if the request processed successfully. */
	SuccessDoNotTrack,       /**< Audio middleware implementations return this if during ExecuteTrigger an event was actually stopped so that internal data can be immediately freed. */
	SuccessNeedsRefresh,     /**< Audio middleware implementations return this if after an action they require to be refreshed. */
	PartialSuccess,          /**< Returned if the outcome of the request wasn't a complete success but also not complete failure. */
	Failure,                 /**< Returned if the request failed to process. */
	Pending,                 /**< Returned if the request was delivered but final execution is pending. It's then kept in the system until its status changed. */
	FailureInvalidControlId, /**< Returned if the request referenced a non-existing audio control. */
	FailureInvalidRequest,   /**< Returned if the request type is unknown/unhandled. */
};

/**
 * @enum CryAudio::ERequestResult
 * @brief A strongly typed enum class representing a list of possible outcomes of a request which gets communicated via the callbacks if the user decided to be informed of the outcome of a particular request.
 * @var CryAudio::ERequestResult::None
 * @var CryAudio::ERequestResult::Success
 * @var CryAudio::ERequestResult::Failure
 */
enum class ERequestResult : EnumFlagsType
{
	None,    /**< Used to initialize variables of this type and to determine whether the variable was properly handled. */
	Success, /**< Set if the request processed successfully. */
	Failure, /**< Set if the request failed to process. */
};

/**
 * @enum CryAudio::EOcclusionType
 * @brief A strongly typed enum class representing different audio occlusion types that can be set on audio objects.
 * @var CryAudio::EOcclusionType::None
 * @var CryAudio::EOcclusionType::Ignore
 * @var CryAudio::EOcclusionType::Adaptive
 * @var CryAudio::EOcclusionType::Low
 * @var CryAudio::EOcclusionType::Medium
 * @var CryAudio::EOcclusionType::High
 * @var CryAudio::EOcclusionType::Count
 */
enum class EOcclusionType : EnumFlagsType
{
	None,     /**< Used to initialize variables of this type and to determine whether the variable was properly handled. */
	Ignore,   /**< The audio object does not calculate occlusion against level geometry. */
	Adaptive, /**< The audio object switches between occlusion types depending on its distance to the audio listener. */
	Low,      /**< The audio object uses a coarse grained occlusion plane for calculation. */
	Medium,   /**< The audio object uses a medium grained occlusion plane for calculation. */
	High,     /**< The audio object uses a fine grained occlusion plane for calculation. */
	Count,    /**< Used to initialize arrays to this size. */
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EOcclusionType);

class CObjectTransformation
{
public:

	CObjectTransformation()
		: m_position(Vec3Constants<float>::fVec3_Zero)
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
		, m_forward(transformation.GetColumn1()) //!< Assuming forward vector = (0,1,0), also assuming unscaled.
		, m_up(transformation.GetColumn2())      //!< Assuming up vector = (0,0,1).
	{
		m_forward.NormalizeFast();
		m_up.NormalizeFast();
	}

	bool IsEquivalent(CObjectTransformation const& transformation, float const epsilon = VEC_EPSILON) const
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

	ILINE Vec3 const&                   GetPosition() const { return m_position; }
	ILINE Vec3 const&                   GetForward() const  { return m_forward; }
	ILINE Vec3 const&                   GetUp() const       { return m_up; }

	static CObjectTransformation const& GetEmptyObject()    { static CObjectTransformation const emptyInstance; return emptyInstance; }

private:

	Vec3 m_position;
	Vec3 m_forward;
	Vec3 m_up;
};

struct SRequestUserData
{
	explicit SRequestUserData(
		ERequestFlags const flags_ = ERequestFlags::None,
		void* const pOwner_ = nullptr,
		void* const pUserData_ = nullptr,
		void* const pUserDataOwner_ = nullptr)
		: flags(flags_)
		, pOwner(pOwner_)
		, pUserData(pUserData_)
		, pUserDataOwner(pUserDataOwner_)
	{}

	static SRequestUserData const& GetEmptyObject() { static SRequestUserData const emptyInstance; return emptyInstance; }

	ERequestFlags const            flags;
	void* const                    pOwner;
	void* const                    pUserData;
	void* const                    pUserDataOwner;
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

struct SImplInfo
{
	CryFixedStringT<MaxInfoStringLength> name;
	CryFixedStringT<MaxInfoStringLength> folderName;
};
} // namespace CryAudio
