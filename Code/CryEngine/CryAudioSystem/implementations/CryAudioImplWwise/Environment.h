// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironment.h>
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

class CEnvironment final : public IEnvironment, public CPoolObject<CEnvironment, stl::PSyncNone>
{
public:

	CEnvironment() = delete;
	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;

	explicit CEnvironment(EEnvironmentType const type_, AkAuxBusID const busId_);

	explicit CEnvironment(
		EEnvironmentType const type_,
		AkRtpcID const rtpcId_,
		float const multiplier_,
		float const shift_);

	virtual ~CEnvironment() override = default;

	EEnvironmentType const type;

	union
	{
		// Aux Bus implementation
		struct
		{
			AkAuxBusID const busId;
		};

		// Rtpc implementation
		struct
		{
			AkRtpcID const rtpcId;
			float const    multiplier;
			float const    shift;
		};
	};
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
