// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
class CAisacEnvironment final : public IEnvironmentConnection, public CPoolObject<CAisacEnvironment, stl::PSyncNone>
{
public:

	CAisacEnvironment() = delete;
	CAisacEnvironment(CAisacEnvironment const&) = delete;
	CAisacEnvironment(CAisacEnvironment&&) = delete;
	CAisacEnvironment& operator=(CAisacEnvironment const&) = delete;
	CAisacEnvironment& operator=(CAisacEnvironment&&) = delete;

	explicit CAisacEnvironment(char const* const szName, float const multiplier, float const shift)
		: m_name(szName)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}

	virtual ~CAisacEnvironment() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
	float const m_multiplier;
	float const m_shift;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
