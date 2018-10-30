// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <vector>
#include <memory>
#include <CryAudio/IAudioSystem.h>

namespace ACE
{
enum class EAssetType : CryAudio::EnumFlagsType
{
	None,
	Trigger,
	Parameter,
	Switch,
	State,
	Environment,
	Preload,
	Setting,
	Folder,
	Library,
	NumTypes };

enum class EPakStatus : CryAudio::EnumFlagsType
{
	None = 0,
	InPak = BIT(0),
	OnDisk = BIT(1), };
CRY_CREATE_ENUM_FLAG_OPERATORS(EPakStatus);

enum class EItemFlags : CryAudio::EnumFlagsType
{
	None = 0,
	IsPlaceHolder = BIT(0),
	IsLocalized = BIT(1),
	IsConnected = BIT(2),
	IsContainer = BIT(3), };
CRY_CREATE_ENUM_FLAG_OPERATORS(EItemFlags);

enum class EErrorCode : CryAudio::EnumFlagsType
{
	None = 0,
	UnkownPlatform = BIT(0), };
CRY_CREATE_ENUM_FLAG_OPERATORS(EErrorCode);

using ControlId = CryAudio::IdType;
static constexpr ControlId s_aceInvalidId = 0;
using ControlIds = std::vector<ControlId>;

class CAsset;
using Assets = std::vector<CAsset*>;

class CControl;
using Controls = std::vector<CControl*>;

class CLibrary;
using Libraries = std::vector<CLibrary*>;

class CFolder;
using Folders = std::vector<CFolder*>;

using Platforms = std::vector<char const*>;
using FileNames = std::set<string>;
using AssetNames = std::vector<string>;

using Scope = uint32;
static constexpr char const* s_szGlobalScopeName = "global";
static constexpr Scope GlobalScopeId = CryAudio::StringToId(s_szGlobalScopeName);

using PlatformIndexType = uint16;

enum class EImplInfoFlags : CryAudio::EnumFlagsType
{
	None = 0,
	SupportsProjects = BIT(0),
	SupportsTriggers = BIT(1),
	SupportsParameters = BIT(2),
	SupportsSwitches = BIT(3),
	SupportsStates = BIT(4),
	SupportsEnvironments = BIT(5),
	SupportsPreloads = BIT(6),
	SupportsSettings = BIT(7), };
CRY_CREATE_ENUM_FLAG_OPERATORS(EImplInfoFlags);

struct SImplInfo final
{
	CryFixedStringT<CryAudio::MaxInfoStringLength> name;
	CryFixedStringT<CryAudio::MaxInfoStringLength> folderName;
	CryFixedStringT<CryAudio::MaxFilePathLength>   projectPath;
	CryFixedStringT<CryAudio::MaxFilePathLength>   assetsPath;
	CryFixedStringT<CryAudio::MaxFilePathLength>   localizedAssetsPath;
	EImplInfoFlags flags;
};
} // namespace ACE
