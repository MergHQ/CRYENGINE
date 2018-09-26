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
class CPlatformConnection final : public CBaseConnection, public CryAudio::CPoolObject<CPlatformConnection, stl::PSyncNone>
{
public:

	CPlatformConnection() = delete;
	CPlatformConnection(CPlatformConnection const&) = delete;
	CPlatformConnection(CPlatformConnection&&) = delete;
	CPlatformConnection& operator=(CPlatformConnection const&) = delete;
	CPlatformConnection& operator=(CPlatformConnection&&) = delete;

	explicit CPlatformConnection(ControlId const id)
		: CBaseConnection(id)
		, m_configurationsMask(std::numeric_limits<PlatformIndexType>::max())
	{}

	virtual ~CPlatformConnection() override = default;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	virtual void SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled) override;
	virtual bool IsPlatformEnabled(PlatformIndexType const platformIndex) const override;
	virtual void ClearPlatforms() override;
	// ~CBaseConnection

private:

	PlatformIndexType m_configurationsMask;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
