// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <CryAudioImplAdx2/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
class CSnapshotConnection final : public CBaseConnection
{
public:

	CSnapshotConnection() = delete;

	explicit CSnapshotConnection(ControlId const id, int const fadeTime = CryAudio::Impl::Adx2::s_defaultChangeoverTime)
		: CBaseConnection(id)
		, m_changeoverTime(fadeTime)
	{}

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	int GetChangeoverTime() const { return m_changeoverTime; }

private:

	int m_changeoverTime;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
