// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
static constexpr ControlId RelativeVelocityParameterId = StringToId(s_szRelativeVelocityParameterName);
static constexpr ControlId AbsoluteVelocityParameterId = StringToId(s_szAbsoluteVelocityParameterName);
static constexpr ControlId LoseFocusTriggerId = StringToId(s_szLoseFocusTriggerName);
static constexpr ControlId GetFocusTriggerId = StringToId(s_szGetFocusTriggerName);
static constexpr ControlId MuteAllTriggerId = StringToId(s_szMuteAllTriggerName);
static constexpr ControlId UnmuteAllTriggerId = StringToId(s_szUnmuteAllTriggerName);
static constexpr ControlId PauseAllTriggerId = StringToId(s_szPauseAllTriggerName);
static constexpr ControlId ResumeAllTriggerId = StringToId(s_szResumeAllTriggerName);

namespace Impl
{
struct IImpl;
} // namespace Impl

class CEventManager;
class CAudioStandaloneFileManager;
class CATLAudioObject;
class CAbsoluteVelocityParameter;
class CRelativeVelocityParameter;
class CLoseFocusTrigger;
class CGetFocusTrigger;
class CMuteAllTrigger;
class CUnmuteAllTrigger;
class CPauseAllTrigger;
class CResumeAllTrigger;

extern Impl::IImpl* g_pIImpl;
extern CEventManager* g_pEventManager;
extern CAudioStandaloneFileManager* g_pFileManager;
extern CATLAudioObject* g_pObject;
extern CAbsoluteVelocityParameter* g_pAbsoluteVelocityParameter;
extern CRelativeVelocityParameter* g_pRelativeVelocityParameter;
extern CLoseFocusTrigger* g_pLoseFocusTrigger;
extern CGetFocusTrigger* g_pGetFocusTrigger;
extern CMuteAllTrigger* g_pMuteAllTrigger;
extern CUnmuteAllTrigger* g_pUnmuteAllTrigger;
extern CPauseAllTrigger* g_pPauseAllTrigger;
extern CResumeAllTrigger* g_pResumeAllTrigger;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
namespace Debug
{
using DebugColor = std::array<float, 4>;

extern DebugColor const g_colorWhite;
extern DebugColor const g_colorRed;
extern DebugColor const g_colorGreen;
extern DebugColor const g_colorBlue;
extern DebugColor const g_colorGreyBright;
extern DebugColor const g_colorOrange;
extern DebugColor const g_colorYellow;
extern DebugColor const g_colorCyan;

// Debug draw style for audio system info.
extern float const g_systemFontSize;
extern float const g_systemLineHeight;
extern float const g_systemLineHeightClause;
extern float const g_systemIndentation;
extern DebugColor const g_systemColorHeader;
extern DebugColor const g_systemColorTextPrimary;
extern DebugColor const g_systemColorTextSecondary;
extern DebugColor const g_systemColorListenerActive;
extern DebugColor const g_systemColorListenerInactive;

// Debug draw style for audio objects.
extern float const g_objectFontSize;
extern float const g_objectLineHeight;
extern float const g_objectRadiusPositionSphere;
extern DebugColor const g_objectColorActive;
extern DebugColor const g_objectColorInactive;
extern DebugColor const g_objectColorVirtual;
extern DebugColor const g_objectColorTrigger;
extern DebugColor const g_objectColorStandaloneFile;
extern DebugColor const g_objectColorParameter;
extern DebugColor const g_objectColorEnvironment;

// Debug draw style for audio object manager, audio event manager, standalone file manager and file cache manager.
extern float const g_managerHeaderFontSize;
extern float const g_managerHeaderLineHeight;
extern float const g_managerFontSize;
extern float const g_managerLineHeight;
extern DebugColor const g_managerColorHeader;
extern DebugColor const g_managerColorItemActive;
extern DebugColor const g_managerColorItemInactive;
extern DebugColor const g_managerColorItemVirtual;
extern DebugColor const g_managerColorItemLoading;
extern DebugColor const g_managerColorItemStopping;

// Debug draw style for rays.
extern float const g_rayRadiusCollisionSphere;
}      // namespace Debug
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
