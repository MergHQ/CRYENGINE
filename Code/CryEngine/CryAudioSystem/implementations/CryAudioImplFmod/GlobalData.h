// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
static constexpr char* s_szImplFolderName = "fmod";

// XML tags
static constexpr char* s_szSnapshotTag = "Snapshot";
static constexpr char* s_szParameterTag = "Parameter";
static constexpr char* s_szFileTag = "File";
static constexpr char* s_szBusTag = "Bus";
static constexpr char* s_szVcaTag = "VCA";

// XML attributes
static constexpr char* s_szValueAttribute = "value";
static constexpr char* s_szMutiplierAttribute = "value_multiplier";
static constexpr char* s_szShiftAttribute = "value_shift";
static constexpr char* s_szLocalizedAttribute = "localized";

// XML values
static constexpr char* s_szTrueValue = "true";
static constexpr char* s_szStopValue = "stop";
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
