// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CImpl;
class CListener;
class CEvent;

extern CImpl* g_pImpl;
extern CListener* g_pListener;

using Events = std::vector<CEvent*>;
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio