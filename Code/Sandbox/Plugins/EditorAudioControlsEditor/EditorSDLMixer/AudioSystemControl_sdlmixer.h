// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioConnection.h>
#include <IAudioSystemItem.h>

namespace ACE
{
enum ESdlMixerTypes
{
	eSdlMixerTypes_Invalid = 0,
	eSdlMixerTypes_Event,
	eSdlMixerTypes_Folder,
};

class IAudioSystemControl_sdlmixer final : public IAudioSystemItem
{
public:
	IAudioSystemControl_sdlmixer() = default;
	IAudioSystemControl_sdlmixer(string const& name, CID const id, ItemType const type);
};
} // namespace ACE
