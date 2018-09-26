// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <PoolObject.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
class CGenericConnection final : public CBaseConnection, public CryAudio::CPoolObject<CGenericConnection, stl::PSyncNone>
{
public:

	CGenericConnection() = delete;
	CGenericConnection(CGenericConnection const&) = delete;
	CGenericConnection(CGenericConnection&&) = delete;
	CGenericConnection& operator=(CGenericConnection const&) = delete;
	CGenericConnection& operator=(CGenericConnection&&) = delete;

	explicit CGenericConnection(ControlId const id)
		: CBaseConnection(id)
	{}

	virtual ~CGenericConnection() override = default;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
