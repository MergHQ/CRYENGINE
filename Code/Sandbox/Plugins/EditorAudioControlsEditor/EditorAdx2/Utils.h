// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Item.h"

namespace ACE
{
namespace Impl
{
namespace Adx2
{
static string const s_binariesFolderName = "Cue Sheet Binaries";
static string const s_workUnitsFolderName = "Work Units";
static string const s_globalSettingsFolderName = "Global Settings";
static string const s_aisacControlsFolderName = "AISAC-Controls";
static string const s_gameVariablesFolderName = "Game Variables";
static string const s_selectorsFolderName = "Selectors";
static string const s_categoriesFolderName = "Categories";
static string const s_dspBusSettingsFolderName = "DSP Bus Settings";
static string const s_dspBusesFolderName = "DSP Buses";
static string const s_snapshotsFolderName = "Snapshots";

namespace Utils
{
ControlId GetId(EItemType const type, string const& name, CItem* const pParent, CItem const& rootItem);
string    GetPathName(CItem const* const pItem, CItem const& rootItem);
string    GetTypeName(EItemType const type);
string    FindCueSheetName(CItem const* const pItem, CItem const& rootItem);
} // namespace Utils
} // namespace Adx2
} // namespace Impl
} // namespace ACE
