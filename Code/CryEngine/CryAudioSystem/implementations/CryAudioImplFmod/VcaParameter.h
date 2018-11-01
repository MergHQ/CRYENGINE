// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseParameter.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CVcaParameter final : public CBaseParameter, public CPoolObject<CVcaParameter, stl::PSyncNone>
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
		FMOD::Studio::VCA* const vca)
		: CBaseParameter(id, multiplier, shift, EParameterType::VCA)
		, m_vca(vca)
	{}

	virtual ~CVcaParameter() override = default;

	FMOD::Studio::VCA* GetVca() const { return m_vca; }

private:

	FMOD::Studio::VCA* const m_vca;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
