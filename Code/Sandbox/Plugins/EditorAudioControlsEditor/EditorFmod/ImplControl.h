// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ImplItem.h>

namespace ACE
{
namespace Fmod
{
enum class EImpltemType
{
	Invalid = 0,
	Folder,
	Event,
	EventParameter,
	Snapshot,
	SnapshotParameter,
	Bank,
	Return,
	Group,
};

class CImplControl final : public CImplItem
{
public:

	CImplControl() = default;
	CImplControl(string const& name, CID const id, ItemType const type);
	virtual ~CImplControl() {}
};

class CImplFolder final : public CImplItem
{
public:

	CImplFolder(string const& name, CID const id)
		: CImplItem(name, id, static_cast<ItemType>(EImpltemType::Folder))
	{}

	virtual bool IsConnected() const override { return true; }
};

class CImplGroup final : public CImplItem
{
public:

	CImplGroup(string const& name, CID const id)
		: CImplItem(name, id, static_cast<ItemType>(EImpltemType::Group))
	{}

	virtual bool IsConnected() const override { return true; }
};
} // namespace Fmod
} // namespace ACE
