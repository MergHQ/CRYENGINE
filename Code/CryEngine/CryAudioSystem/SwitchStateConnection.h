// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"

namespace CryAudio
{
class CObject;

namespace Impl
{
struct ISwitchState;
} // namespace Impl

class CSwitchStateConnection final : public CPoolObject<CSwitchStateConnection, stl::PSyncNone>
{
public:

	CSwitchStateConnection() = delete;
	CSwitchStateConnection(CSwitchStateConnection const&) = delete;
	CSwitchStateConnection(CSwitchStateConnection&&) = delete;
	CSwitchStateConnection& operator=(CSwitchStateConnection const&) = delete;
	CSwitchStateConnection& operator=(CSwitchStateConnection&&) = delete;

	explicit CSwitchStateConnection(Impl::ISwitchState const* const pImplData)
		: m_pImplData(pImplData)
	{}

	virtual ~CSwitchStateConnection();

	void Set(CObject const& object) const;

private:

	Impl::ISwitchState const* const m_pImplData = nullptr;
};
} // namespace CryAudio
