// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLEntities.h"

#if CRY_PLATFORM_WINDOWS
char const* const SATLXMLTags::szPlatform = "pc";
#elif CRY_PLATFORM_DURANGO
char const* const SATLXMLTags::szPlatform = "xboxone";
#elif CRY_PLATFORM_ORBIS
char const* const SATLXMLTags::szPlatform = "ps4";
#elif CRY_PLATFORM_MAC
char const* const SATLXMLTags::szPlatform = "mac";
#elif CRY_PLATFORM_LINUX
char const* const SATLXMLTags::szPlatform = "linux";
#elif CRY_PLATFORM_IOS
char const* const SATLXMLTags::szPlatform = "ios";
#elif CRY_PLATFORM_ANDROID
char const* const SATLXMLTags::szPlatform = "linux";
#else
	#error "Undefined platform."
#endif

char const* const SATLXMLTags::szRootNodeTag = "ATLConfig";
char const* const SATLXMLTags::szEditorDataTag = "EditorData";
char const* const SATLXMLTags::szTriggersNodeTag = "AudioTriggers";
char const* const SATLXMLTags::szRtpcsNodeTag = "AudioRtpcs";
char const* const SATLXMLTags::szSwitchesNodeTag = "AudioSwitches";
char const* const SATLXMLTags::szPreloadsNodeTag = "AudioPreloads";
char const* const SATLXMLTags::szEnvironmentsNodeTag = "AudioEnvironments";

char const* const SATLXMLTags::szATLTriggerTag = "ATLTrigger";
char const* const SATLXMLTags::szATLSwitchTag = "ATLSwitch";
char const* const SATLXMLTags::szATLRtpcTag = "ATLRtpc";
char const* const SATLXMLTags::szATLSwitchStateTag = "ATLSwitchState";
char const* const SATLXMLTags::szATLEnvironmentTag = "ATLEnvironment";
char const* const SATLXMLTags::szATLPlatformsTag = "ATLPlatforms";
char const* const SATLXMLTags::szATLConfigGroupTag = "ATLConfigGroup";

char const* const SATLXMLTags::szATLTriggerRequestTag = "ATLTriggerRequest";
char const* const SATLXMLTags::szATLSwitchRequestTag = "ATLSwitchRequest";
char const* const SATLXMLTags::szATLValueTag = "ATLValue";
char const* const SATLXMLTags::szATLRtpcRequestTag = "ATLRtpcRequest";
char const* const SATLXMLTags::szATLPreloadRequestTag = "ATLPreloadRequest";
char const* const SATLXMLTags::szATLEnvironmentRequestTag = "ATLEnvironmentRequest";

char const* const SATLXMLTags::szATLNameAttribute = "atl_name";
char const* const SATLXMLTags::szATLVersionAttribute = "atl_version";
char const* const SATLXMLTags::szATLInternalNameAttribute = "atl_internal_name";
char const* const SATLXMLTags::szATLTypeAttribute = "atl_type";
char const* const SATLXMLTags::szATLConfigGroupAttribute = "atl_config_group_name";
char const* const SATLXMLTags::szATLRadiusAttribute = "atl_radius";
char const* const SATLXMLTags::szATLOcclusionFadeOutDistanceAttribute = "atl_occlusion_fadeout_distance";

char const* const SATLXMLTags::szATLDataLoadType = "AutoLoad";

AudioControlId SATLInternalControlIDs::obstructionOcclusionCalcSwitchId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::objectDopplerTrackingSwitchId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::objectVelocityTrackingSwitchId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::loseFocusTriggerId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::getFocusTriggerId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::muteAllTriggerId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::unmuteAllTriggerId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::objectDopplerRtpcId = INVALID_AUDIO_CONTROL_ID;
AudioControlId SATLInternalControlIDs::objectVelocityRtpcId = INVALID_AUDIO_CONTROL_ID;
AudioSwitchStateId SATLInternalControlIDs::ignoreStateId = INVALID_AUDIO_SWITCH_STATE_ID;
AudioSwitchStateId SATLInternalControlIDs::adaptiveStateId = INVALID_AUDIO_SWITCH_STATE_ID;
AudioSwitchStateId SATLInternalControlIDs::lowStateId = INVALID_AUDIO_SWITCH_STATE_ID;
AudioSwitchStateId SATLInternalControlIDs::mediumStateId = INVALID_AUDIO_SWITCH_STATE_ID;
AudioSwitchStateId SATLInternalControlIDs::highStateId = INVALID_AUDIO_SWITCH_STATE_ID;
AudioSwitchStateId SATLInternalControlIDs::onStateId = INVALID_AUDIO_SWITCH_STATE_ID;
AudioSwitchStateId SATLInternalControlIDs::offStateId = INVALID_AUDIO_SWITCH_STATE_ID;
AudioPreloadRequestId SATLInternalControlIDs::globalPreloadRequestId = INVALID_AUDIO_PRELOAD_REQUEST_ID;

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
void InitATLControlIDs()
{
	using namespace CryAudio::Impl;
	SATLInternalControlIDs::loseFocusTriggerId = AudioStringToId("lose_focus");
	SATLInternalControlIDs::getFocusTriggerId = AudioStringToId("get_focus");
	SATLInternalControlIDs::muteAllTriggerId = AudioStringToId("mute_all");
	SATLInternalControlIDs::unmuteAllTriggerId = AudioStringToId("unmute_all");
	SATLInternalControlIDs::objectDopplerRtpcId = AudioStringToId("object_doppler");
	SATLInternalControlIDs::objectVelocityRtpcId = AudioStringToId("object_speed");
	SATLInternalControlIDs::obstructionOcclusionCalcSwitchId = AudioStringToId("ObstructionOcclusionCalculationType");
	SATLInternalControlIDs::ignoreStateId = AudioStringToId("ignore");
	SATLInternalControlIDs::adaptiveStateId = AudioStringToId("adaptive");
	SATLInternalControlIDs::lowStateId = AudioStringToId("low");
	SATLInternalControlIDs::mediumStateId = AudioStringToId("medium");
	SATLInternalControlIDs::highStateId = AudioStringToId("high");
	SATLInternalControlIDs::objectDopplerTrackingSwitchId = AudioStringToId("object_doppler_tracking");
	SATLInternalControlIDs::objectVelocityTrackingSwitchId = AudioStringToId("object_velocity_tracking");
	SATLInternalControlIDs::onStateId = AudioStringToId("on");
	SATLInternalControlIDs::offStateId = AudioStringToId("off");
	SATLInternalControlIDs::globalPreloadRequestId = AudioStringToId("global_atl_preloads");
}
