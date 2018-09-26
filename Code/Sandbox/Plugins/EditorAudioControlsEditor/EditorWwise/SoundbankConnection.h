// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <PoolObject.h>

namespace ACE
{
namespace Impl
{
namespace Wwise
{
class CSoundbankConnection final : public CBaseConnection, public CryAudio::CPoolObject<CSoundbankConnection, stl::PSyncNone>
{
public:

	CSoundbankConnection() = delete;
	CSoundbankConnection(CSoundbankConnection const&) = delete;
	CSoundbankConnection(CSoundbankConnection&&) = delete;
	CSoundbankConnection& operator=(CSoundbankConnection const&) = delete;
	CSoundbankConnection& operator=(CSoundbankConnection&&) = delete;

	explicit CSoundbankConnection(ControlId const id)
		: CBaseConnection(id)
		, m_configurationsMask(std::numeric_limits<PlatformIndexType>::max())
	{}

	virtual ~CSoundbankConnection() override = default;

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
} // namespace Wwise
} // namespace Impl
} // namespace ACE
