// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ImplControls.h"

namespace ACE
{
namespace Fmod
{
static string const g_soundBanksFolderName = "Banks";
static string const g_eventsFolderName = "Events";
static string const g_parametersFolderName = "Parameters";
static string const g_snapshotsFolderName = "Snapshots";
static string const g_returnsFolderName = "Returns";
static string const g_vcasFolderName = "VCAs";

namespace Utils
{
CID    GetId(EImplItemType const type, string const& name, CImplItem* const pParent, CImplItem const& rootControl);
string GetPathName(CImplItem const* const pImplItem, CImplItem const& rootControl);
string GetTypeName(EImplItemType const type);
} // namespace Utils
} // namespace Fmod
} // namespace ACE
