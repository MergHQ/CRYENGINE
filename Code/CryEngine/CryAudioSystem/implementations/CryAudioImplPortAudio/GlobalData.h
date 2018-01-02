// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
static constexpr char* s_szImplFolderName = "portaudio";

// XML tags
static constexpr char* s_szFileTag = "Sample";

// XML attributes
static constexpr char* s_szPathAttribute = "path";
static constexpr char* s_szLoopCountAttribute = "loop_count";

// XML values
static constexpr char* s_szStartValue = "start";
static constexpr char* s_szStopValue = "stop";
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
