// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Item.h"

namespace ACE
{
namespace Impl
{
namespace PortAudio
{
namespace Utils
{
ControlId GetId(EItemType const type, string const& name, string const& path, bool const isLocalized);
string    GetTypeName(EItemType const type);
} // namespace Utils
} // namespace PortAudio
} // namespace Impl
} // namespace ACE
