// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <PoolObject.h>

namespace ACE
{
namespace Impl
{
namespace Fmod
{
class CBankConnection final : public CBaseConnection, public CryAudio::CPoolObject<CBankConnection, stl::PSyncNone>
{
public:

	CBankConnection() = delete;
	CBankConnection(CBankConnection const&) = delete;
	CBankConnection(CBankConnection&&) = delete;
	CBankConnection& operator=(CBankConnection const&) = delete;
	CBankConnection& operator=(CBankConnection&&) = delete;

	explicit CBankConnection(ControlId const id)
		: CBaseConnection(id)
		, m_configurationsMask(std::numeric_limits<PlatformIndexType>::max())
	{}

	virtual ~CBankConnection() override = default;

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
} // namespace Fmod
} // namespace Impl
} // namespace ACE
