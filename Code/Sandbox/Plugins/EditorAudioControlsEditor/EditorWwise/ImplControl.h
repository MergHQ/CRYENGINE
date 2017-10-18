// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ImplItem.h>

namespace ACE
{
namespace Wwise
{
enum class EImpltemType
{
	Invalid = 0,
	Event,
	Parameter,
	Switch,
	AuxBus,
	SoundBank,
	State,
	SwitchGroup,
	StateGroup,
	WorkUnit,
	VirtualFolder,
	PhysicalFolder,
};

class CImplControl final : public CImplItem
{
public:

	CImplControl() = default;
	CImplControl(string const& name, CID const id, ItemType const type);
	virtual ~CImplControl() {}
};
} // namespace Wwise
} // namespace ACE
