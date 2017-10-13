// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "AudioSystemControl_sdlmixer.h"

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
IAudioSystemControl_sdlmixer::IAudioSystemControl_sdlmixer(string const& name, CID const id, ItemType const type)
	: IAudioSystemItem(name, id, type)
{
}
} // namespace ACE
