// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <CryString/CryString.h>

namespace ACE
{
struct IConnection;

namespace AssetUtils
{
string      GenerateUniqueName(string const& name, EAssetType const type, CAsset* const pAsset, CAsset* const pParent);
string      GenerateUniqueLibraryName(string const& name, CAsset* const pAsset);
string      GenerateUniqueControlName(string const& name, EAssetType const type, CControl* const pControl);
ControlId   GenerateUniqueAssetId(string const& name, EAssetType const type);
ControlId   GenerateUniqueStateId(string const& switchName, string const& stateName);
ControlId   GenerateUniqueFolderId(string const& name, CAsset* const pParent);
CAsset*     GetParentLibrary(CAsset* const pAsset);
char const* GetTypeName(EAssetType const type);
void        SelectTopLevelAncestors(Assets const& source, Assets& dest);
void        TryConstructTriggerConnectionNode(
	XmlNodeRef const& triggerNode,
	IConnection const* const pIConnection,
	CryAudio::ContextId const contextId);
} // namespace AssetUtils
} // namespace ACE
