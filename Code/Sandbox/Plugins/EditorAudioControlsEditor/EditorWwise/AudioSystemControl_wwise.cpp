// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "AudioSystemControl_wwise.h"

namespace ACE
{
IAudioSystemControl_wwise::IAudioSystemControl_wwise(const string& name, CID id, ItemType type)
	: IAudioSystemItem(name, id, type)
{
}
}
