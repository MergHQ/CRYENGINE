// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioConnection.h>
#include <IAudioSystemItem.h>

namespace ACE
{
enum EWwiseItemTypes
{
	eWwiseItemTypes_Invalid = 0,
	eWwiseItemTypes_Event,
	eWwiseItemTypes_Parameter,
	eWwiseItemTypes_Switch,
	eWwiseItemTypes_AuxBus,
	eWwiseItemTypes_SoundBank,
	eWwiseItemTypes_State,
	eWwiseItemTypes_SwitchGroup,
	eWwiseItemTypes_StateGroup,
	eWwiseItemTypes_WorkUnit,
	eWwiseItemTypes_VirtualFolder,
	eWwiseItemTypes_PhysicalFolder,
};

class IAudioSystemControl_wwise final : public IAudioSystemItem
{
public:

	IAudioSystemControl_wwise() {}
	IAudioSystemControl_wwise(string const& name, CID const id, ItemType const type);
	virtual ~IAudioSystemControl_wwise() {}
};
} // namespace ACE
