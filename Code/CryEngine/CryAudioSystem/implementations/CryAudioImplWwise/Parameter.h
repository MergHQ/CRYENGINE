// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CParameter final : public IParameter
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	explicit CParameter(AkRtpcID const id_, float const mult_, float const shift_);
	virtual ~CParameter() override = default;

	float const    mult;
	float const    shift;
	AkRtpcID const id;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
