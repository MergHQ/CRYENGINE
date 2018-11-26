// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseSwitchState.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CVcaState final : public CBaseSwitchState, public CPoolObject<CVcaState, stl::PSyncNone>
{
public:

	CVcaState() = delete;
	CVcaState(CVcaState const&) = delete;
	CVcaState(CVcaState&&) = delete;
	CVcaState& operator=(CVcaState const&) = delete;
	CVcaState& operator=(CVcaState&&) = delete;

	explicit CVcaState(
		uint32 const id,
		float const value,
		FMOD::Studio::VCA* const pVca)
		: CBaseSwitchState(id, value, EStateType::VCA)
		, m_pVca(pVca)
	{}

	virtual ~CVcaState() override = default;

	// ISwitchStateConnection
	virtual void Set(IObject* const pIObject) override;
	// ~ISwitchStateConnection

private:

	FMOD::Studio::VCA* const m_pVca;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
