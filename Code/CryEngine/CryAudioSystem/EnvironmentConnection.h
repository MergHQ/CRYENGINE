// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"

namespace CryAudio
{
namespace Impl
{
struct IEnvironment;
} // namespace Impl

class CEnvironmentConnection final : public CPoolObject<CEnvironmentConnection, stl::PSyncNone>
{
public:

	explicit CEnvironmentConnection(Impl::IEnvironment const* const pImplData)
		: m_pImplData(pImplData)
	{}

	~CEnvironmentConnection();

	Impl::IEnvironment const* const m_pImplData;
};
} // namespace CryAudio
