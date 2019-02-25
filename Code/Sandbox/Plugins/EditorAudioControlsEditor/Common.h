// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/SharedData.h"

namespace ACE
{
namespace Impl
{
struct IImpl;
} // namespace Impl

class CAssetsManager;
class CImplementationManager;
class CAsset;
class CControl;
class CLibrary;
class CFolder;
class CMainWindow;
class CSystemControlsWidget;
class CPropertiesWidget;
class CMiddlewareDataWidget;
class CFileMonitorMiddleware;

using Assets = std::vector<CAsset*>;
using Controls = std::vector<CControl*>;
using Libraries = std::vector<CLibrary*>;
using Folders = std::vector<CFolder*>;
using FileNames = std::set<string>;
using AssetNames = std::vector<string>;

extern CAssetsManager g_assetsManager;
extern CImplementationManager g_implementationManager;
extern Impl::IImpl* g_pIImpl;
extern CMainWindow* g_pMainWindow;
extern CSystemControlsWidget* g_pSystemControlsWidget;
extern CPropertiesWidget* g_pPropertiesWidget;
extern CMiddlewareDataWidget* g_pMiddlewareDataWidget;
extern CFileMonitorMiddleware* g_pFileMonitorMiddleware;

extern SImplInfo g_implInfo;
extern Platforms g_platforms;

constexpr uint8 g_currentFileVersion = 4;

constexpr char const* g_szLibraryNodeTag = "Library";
constexpr char const* g_szFoldersNodeTag = "Folders";
constexpr char const* g_szControlsNodeTag = "Controls";
constexpr char const* g_szFolderTag = "Folder";
constexpr char const* g_szPathAttribute = "path";
constexpr char const* g_szDescriptionAttribute = "description";

using Scope = uint32;
constexpr char const* g_szGlobalScopeName = "global";
constexpr Scope g_globalScopeId = CryAudio::StringToId(g_szGlobalScopeName);

extern ControlIds g_importedItemIds;

enum class EErrorCode : CryAudio::EnumFlagsType
{
	None = 0,
	UnkownPlatform = BIT(0), };
CRY_CREATE_ENUM_FLAG_OPERATORS(EErrorCode);
} // namespace ACE
