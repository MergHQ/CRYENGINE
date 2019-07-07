// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironmentConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CDspBus final : public IEnvironmentConnection, public CPoolObject<CDspBus, stl::PSyncNone>
{
public:

	CDspBus() = delete;
	CDspBus(CDspBus const&) = delete;
	CDspBus(CDspBus&&) = delete;
	CDspBus& operator=(CDspBus const&) = delete;
	CDspBus& operator=(CDspBus&&) = delete;

	explicit CDspBus(char const* const szName)
		: m_name(szName)
	{}

	virtual ~CDspBus() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
