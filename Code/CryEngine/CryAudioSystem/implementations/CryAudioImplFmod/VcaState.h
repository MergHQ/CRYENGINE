// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SwitchState.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CVcaState final : public CSwitchState
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
		char const* const szName,
		FMOD::Studio::VCA* const vca);

	virtual ~CVcaState() override = default;

	FMOD::Studio::VCA* GetVca() const { return m_vca; }

private:

	FMOD::Studio::VCA* const m_vca;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
