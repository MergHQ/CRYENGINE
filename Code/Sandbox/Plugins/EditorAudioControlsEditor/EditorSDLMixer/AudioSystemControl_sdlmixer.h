// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioConnection.h>
#include <IAudioSystemItem.h>

namespace ACE
{
enum ESDLMixerTypes
{
	eSDLMixerTypes_Invalid    = 0,
	eSDLMixerTypes_Event      = 1,
	eSDLMixerTypes_SampleFile = 2,
};

class IAudioSystemControl_sdlmixer : public IAudioSystemItem
{

public:
	IAudioSystemControl_sdlmixer() {}
	IAudioSystemControl_sdlmixer(const string& name, CID id, ItemType type);
	virtual ~IAudioSystemControl_sdlmixer() {}
};
}
