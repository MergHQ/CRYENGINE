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
		FMOD::Studio::VCA* const pVca)
		: CBaseParameter(id, multiplier, shift, EParameterType::VCA)
		, m_pVca(pVca)
	{}

	virtual ~CVcaParameter() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	// ~IParameterConnection

private:

	FMOD::Studio::VCA* const m_pVca;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
