// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Common/IConnection.h"
#include <PoolObject.h>

namespace ACE
{
namespace Impl
{
namespace SDLMixer
{
class CPreloadConnection final : public IConnection, public CryAudio::CPoolObject<CPreloadConnection, stl::PSyncNone>
{
public:

	CPreloadConnection() = delete;
	CPreloadConnection(CPreloadConnection const&) = delete;
	CPreloadConnection(CPreloadConnection&&) = delete;
	CPreloadConnection& operator=(CPreloadConnection const&) = delete;
	CPreloadConnection& operator=(CPreloadConnection&&) = delete;

	explicit CPreloadConnection(ControlId const id)
		: m_id(id)
	{}

	virtual ~CPreloadConnection() override = default;

	// CBaseConnection
	virtual ControlId GetID() const override                          { return m_id; }
	virtual bool      HasProperties() const override                  { return false; }
	virtual void      Serialize(Serialization::IArchive& ar) override {}
	// ~CBaseConnection

private:

	ControlId const m_id;
};
} // namespace SDLMixer
} // namespace Impl
} // namespace ACE
