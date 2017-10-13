// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioConnection.h>
#include <IAudioSystemItem.h>

namespace ACE
{
enum EPortAudioTypes
{
	ePortAudioTypes_Invalid = 0,
	ePortAudioTypes_Event,
	ePortAudioTypes_Folder,
};

class IAudioSystemControl final : public IAudioSystemItem
{
public:

	IAudioSystemControl() = default;
	IAudioSystemControl(string const& name, CID const id, ItemType const type);
	virtual ~IAudioSystemControl() {}
};
} // namespace ACE
