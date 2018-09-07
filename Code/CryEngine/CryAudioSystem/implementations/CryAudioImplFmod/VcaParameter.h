// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Parameter.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CVcaParameter final : public CParameter
{
public:

	CVcaParameter() = delete;
	CVcaParameter(CVcaParameter const&) = delete;
	CVcaParameter(CVcaParameter&&) = delete;
	CVcaParameter& operator=(CVcaParameter const&) = delete;
	CVcaParameter& operator=(CVcaParameter&&) = delete;

	explicit CVcaParameter(
		uint32 const id,
		float const multiplier,
		float const shift,
		char const* const szName,
		FMOD::Studio::VCA* const vca);

	virtual ~CVcaParameter() override = default;

	FMOD::Studio::VCA* GetVca() const { return m_vca; }

private:

	FMOD::Studio::VCA* const m_vca;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
