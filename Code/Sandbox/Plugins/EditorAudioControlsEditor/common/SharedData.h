// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <vector>
#include <memory>
#include <CryCore/CryEnumMacro.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace ACE
{
enum class EAssetType
{
	None,
	Trigger,
	Parameter,
	Switch,
	State,
	Environment,
	Preload,
	Folder,
	Library,
	NumTypes
};

using ControlId = CryAudio::IdType;
static ControlId const s_aceInvalidId = 0;

struct IConnection;
using ConnectionPtr = std::shared_ptr<IConnection>;

using Platforms = std::vector<char const*>;
using FileNames = std::set<string>;

// Available levels where the controls can be stored.
struct SScopeInfo
{
	SScopeInfo() = default;
	SScopeInfo(string const& name_, bool const isLocalOnly_)
		: name(name_)
		, isLocalOnly(isLocalOnly_)
	{}

	string name;

	// If true, there is a level in the game audio data that doesn't
	// exist in the global list of levels for your project.
	bool isLocalOnly;
};

using Scope = uint32;
using ScopeInfoList = std::vector<SScopeInfo>;

enum class EErrorCode
{
	None                     = 0,
	UnkownPlatform           = BIT(0),
	NonMatchedActivityRadius = BIT(1),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EErrorCode);
} // namespace ACE

