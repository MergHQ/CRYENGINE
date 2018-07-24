// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
class CTrigger;
class CParameter;
class CATLSwitch;
class CATLPreloadRequest;
class CATLAudioEnvironment;

using AudioTriggerLookup = std::map<ControlId, CTrigger const*>;
using AudioParameterLookup = std::map<ControlId, CParameter const*>;
using AudioSwitchLookup = std::map<ControlId, CATLSwitch const*>;
using AudioPreloadRequestLookup = std::map<PreloadRequestId, CATLPreloadRequest*>;
using AudioEnvironmentLookup = std::map<EnvironmentId, CATLAudioEnvironment const*>;
} // namespace CryAudio
