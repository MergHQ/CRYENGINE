// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/SharedData.h"

namespace ACE
{
namespace Impl
{
struct IImpl;
} // namespace Impl

class CSystemControlsWidget;
class CPropertiesWidget;
class CMiddlewareDataWidget;

extern Impl::IImpl* g_pIImpl;
extern CSystemControlsWidget* g_pSystemControlsWidget;
extern CPropertiesWidget* g_pPropertiesWidget;
extern CMiddlewareDataWidget* g_pMiddlewareDataWidget;

extern SImplInfo g_implInfo;
extern Platforms g_platforms;

static constexpr char const* s_szLibraryNodeTag = "Library";
static constexpr char const* s_szFoldersNodeTag = "Folders";
static constexpr char const* s_szControlsNodeTag = "Controls";
static constexpr char const* s_szFolderTag = "Folder";
static constexpr char const* s_szPathAttribute = "path";
static constexpr char const* s_szDescriptionAttribute = "description";
} // namespace ACE
