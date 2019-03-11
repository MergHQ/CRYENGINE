// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <CryString/CryString.h>

namespace ACE
{
class CAsset;
struct IConnection;

namespace AssetUtils
{
string      GenerateUniqueName(string const& name, EAssetType const type, CAsset* const pParent);
string      GenerateUniqueLibraryName(string const& name);
string      GenerateUniqueControlName(string const& name, EAssetType const type);
CAsset*     GetParentLibrary(CAsset* const pAsset);
char const* GetTypeName(EAssetType const type);
void        SelectTopLevelAncestors(Assets const& source, Assets& dest);
void        TryConstructTriggerConnectionNode(
	XmlNodeRef const pTriggerNode,
	IConnection const* const pIConnection,
	CryAudio::ContextId const contextId);
} // namespace AssetUtils
} // namespace ACE
