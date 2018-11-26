// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironmentConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
enum class EEnvironmentType : EnumFlagsType
{
	None,
	AuxBus,
	Rtpc,
};

class CEnvironment final : public IEnvironmentConnection, public CPoolObject<CEnvironment, stl::PSyncNone>
{
public:

	CEnvironment() = delete;
	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;

	explicit CEnvironment(EEnvironmentType const type, AkAuxBusID const busId);

	explicit CEnvironment(
		EEnvironmentType const type,
		AkRtpcID const rtpcId,
		float const multiplier,
		float const shift);

	virtual ~CEnvironment() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	EEnvironmentType const m_type;

	union
	{
		// Aux Bus implementation
		struct
		{
			AkAuxBusID const m_busId;
		};

		// Rtpc implementation
		struct
		{
			AkRtpcID const m_rtpcId;
			float const    m_multiplier;
			float const    m_shift;
		};
	};
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
