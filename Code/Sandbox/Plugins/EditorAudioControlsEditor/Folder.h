// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Asset.h"

#include <PoolObject.h>

namespace ACE
{
class CFolder final : public CAsset, public CryAudio::CPoolObject<CFolder, stl::PSyncNone>
{
public:

	explicit CFolder(string const& name);

	CFolder() = delete;
};
} // namespace ACE
