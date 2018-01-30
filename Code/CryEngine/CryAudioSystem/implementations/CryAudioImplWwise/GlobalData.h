// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
static constexpr char* s_szImplFolderName = "wwise";

// XML tags
static constexpr char* s_szParameterTag = "Parameter";
static constexpr char* s_szSwitchGroupTag = "SwitchGroup";
static constexpr char* s_szStateGroupTag = "StateGroup";
static constexpr char* s_szFileTag = "File";
static constexpr char* s_szAuxBusTag = "AuxBus";
static constexpr char* s_szValueTag = "Value";

// XML attributes
static constexpr char* s_szValueAttribute = "value";
static constexpr char* s_szMutiplierAttribute = "value_multiplier";
static constexpr char* s_szShiftAttribute = "value_shift";
static constexpr char* s_szLocalizedAttribute = "localized";

// XML values
static constexpr char* s_szTrueValue = "true";
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
