// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Item.h"

namespace ACE
{
namespace Impl
{
namespace SDLMixer
{
namespace Utils
{
ControlId GetId(EItemType const type, string const& name, string const& path, bool const isLocalized);
string    GetTypeName(EItemType const type);
} // namespace Utils
} // namespace SDLMixer
} // namespace Impl
} // namespace ACE
